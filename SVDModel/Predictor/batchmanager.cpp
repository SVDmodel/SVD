#include "batchmanager.h"
#include "batch.h"
#include "tensorhelper.h"

#include "model.h"
#include "settings.h"

#include <mutex>


#include "strtools.h"

BatchManager *BatchManager::mInstance = nullptr;

static std::map< std::string, InputTensorItem::DataContent> data_contents = {
    {"Invalid",       InputTensorItem::Invalid},
    {"Climate",       InputTensorItem::Climate},
    {"State",         InputTensorItem::State},
    {"ResidenceTime", InputTensorItem::ResidenceTime},
    {"Neighbors",     InputTensorItem::Neighbors},
    {"Site",          InputTensorItem::Site},
    {"Scalar",          InputTensorItem::Scalar},
    {"DistanceOutside", InputTensorItem::DistanceOutside}
};
static std::map< std::string, InputTensorItem::DataType> data_types = {
    {"Invalid", InputTensorItem::DT_INVALID},
    {"float",   InputTensorItem::DT_FLOAT},
    {"int16",   InputTensorItem::DT_INT16},
    {"int64",   InputTensorItem::DT_INT64},
    {"uint16",  InputTensorItem::DT_UINT16},
    {"float16", InputTensorItem::DT_BFLOAT16},
    {"bool",    InputTensorItem::DT_BOOL}
};

template <typename T>
std::string keys_to_string(const std::map<std::string, T> &mp) {
    std::vector<std::string> s;
    for (auto it : mp)
        s.push_back(it.first);
    return join(s, ",");
}





BatchManager::BatchManager()
{
    if (mInstance!=nullptr)
        throw std::logic_error("Creation of batch manager: instance ptr is not 0.");
    mInstance = this;
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("Batch manager created: {}", static_cast<void*>(this));

}

BatchManager::~BatchManager()
{
    // delete all batches and free memory
    for (auto b : mBatches)
        delete b;

    if (auto lg = spdlog::get("dnn"))
        lg->debug("Batch manager destroyed: {x}", static_cast<void*>(this));

    mInstance = nullptr;
}

void BatchManager::setup()
{

    lg = spdlog::get("dnn");
    if (!lg)
        throw std::logic_error("BatchManager::setup: logging not available.");
    lg->info("Setup of batch manager.");
    Model::instance()->settings().requiredKeys("dnn", {"batchSize", "maxBatchQueue"});
    mBatchSize = static_cast<size_t>(Model::instance()->settings().valueInt("dnn.batchSize"));
    mMaxQueueLength = static_cast<size_t>(Model::instance()->settings().valueInt("dnn.maxBatchQueue"));

//    mTensorDef =  {
//        {"test", InputTensorItem::DT_FLOAT, 2, 24, 10, InputTensorItem::Climate},
//        {"test2", InputTensorItem::DT_INT16, 1, 1, 0, InputTensorItem::State}
//    };
    mTensorDef =  {
        // {"clim_input", "float", 2, 10, 40, "Climate"}, // GPP Climate
        {"clim_input", "float", 2, 10, 24, "Climate"}, // monthly climate
        {"state_input", "int16", 1, 1, 0, "State"},
        {"time_input", "float", 1, 1, 0, "ResidenceTime"},
        {"site_input", "float", 1, 2, 0, "Site"} ,
        {"distance_input", "float", 1, 1, 0, "DistanceOutside"}, // distance to the forested area outside
        {"neighbor_input", "float", 1, 62, 0, "Neighbors"},
        {"keras_learning_phase", "bool", 0, 0, 0, "Scalar"}
    };


    if (lg->should_log(spdlog::level::debug)) {
        lg->debug("Available data types: {}", keys_to_string(data_types));
        lg->debug("Available content types: {}", keys_to_string(data_contents));
        // print tensor-items
        lg->debug("InputTensorItems:");
        for (auto &i : mTensorDef)
            lg->debug("Name: '{}', dataype: '{}', dimensions: {}, size-x: {}, size-y: {}, content: '{}'",
                      i.name, i.datatypeString(i.type), i.ndim, i.sizeX, i.sizeY, i.contentString(i.content));
    }

}

void BatchManager::newYear()
{
    mSlotRequested = false;
}

static std::mutex batch_mutex;
std::pair<Batch *, size_t> BatchManager::validSlot()
{
    // serialize this function...
    std::lock_guard<std::mutex> guard(batch_mutex);
    mSlotRequested = true;
    std::pair<Batch *, int> result;
    int sleeps = 0;
    do {
        result = findValidSlot();
        if (!result.first) {
            // wait


            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            Model::instance()->processEvents();

            if (++sleeps % 100 == 0) // 1s
                lg->trace("BatchManager: no batch available (queue full). Sleeping for {} s.", sleeps/100);

            if (RunState::instance()->cancel() ) {
                lg->info("Canceled.");
                return std::pair<Batch*, int>(nullptr, 0);

            }
            if ( sleeps % 10000 == 0) { // 100 secs
                lg->error("time out in batch manager - no empty slots found.");
                return std::pair<Batch*, int>(nullptr, -1);

            }
        }
    } while (!result.first);
    return result;

}

std::pair<Batch *, int> BatchManager::findValidSlot()
{
    // this function is serialized (access via validSlot() ).

    // look for a batch which is currently not in the DNN processing chain
    Batch *batch = nullptr;
    for (const auto &b : mBatches) {
        if (b->state()==Batch::Fill && b->freeSlots()>0) {
            batch=b;
            break;
        }
    }
    if (!batch || batch->freeSlots()<=0) {
        if (mBatches.size() >= mMaxQueueLength) {
            // currently we don't find a proper place for the data.
            return std::pair<Batch*, int>(nullptr, 0);
        }
        batch = createBatch();
        mBatches.push_back( batch );
        lg->trace("created a new batch. Now the list contains {} batch(es).", mBatches.size());
        /*if ( lg->should_log(spdlog::level::trace) ) {
            int idx=0;
            for (auto b : mBatches) {
                lg->trace("#{}: state: {}, used: {}, free: {}", idx, b->state(), b->usedSlots(), b->freeSlots());
                ++idx;
            }
        }*/
    }


    // get a new slot in the batch
    std::pair<Batch *, size_t> result;
    result.first = batch;
    result.second = batch->acquireSlot();
    if (result.second==0) {
        lg->debug("Started to fill batch {} [{}] (first slot acquired)", batch->packageId(), static_cast<void*>(batch));
    }
    return result;


}

TensorWrapper *BatchManager::buildTensor(size_t batch_size, InputTensorItem &item)
{
    TensorWrapper *tw = nullptr;

    // a scalar, i.e. one value for the whole *batch*
    if (item.ndim == 0) {
        switch (item.type) {
        case InputTensorItem::DT_BOOL: {
            tw = new TensorWrap1d<bool>();
            // defaults to true, TODO
            TensorWrap1d<bool> *twb = static_cast< TensorWrap1d<bool>* >(tw);
            twb->setValue(false);
            lg->debug("created a scalar, value: '{}'", twb->value());
            break;
        }
        default: break;
        }

    }

    // a 1d vector per example (or a single value per example)
    if (item.ndim == 1) {
        switch (item.type) {
        case InputTensorItem::DT_FLOAT:
            tw = new TensorWrap2d<float>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_INT16:
            tw = new TensorWrap2d<short int>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_UINT16:
            tw = new TensorWrap2d<short unsigned int>(batch_size, item.sizeX); break;
        case InputTensorItem::DT_INT64:
            tw = new TensorWrap2d<long long>(batch_size, item.sizeX); break;
        default:
            throw std::logic_error("Unhandled data type in tensorwrapper");
        }
    }

    // a 2d vector by example
    if (item.ndim==2) {
        switch (item.type) {
        case InputTensorItem::DT_FLOAT:
            tw = new TensorWrap3d<float>(batch_size, item.sizeX, item.sizeY); break;
        default: throw std::logic_error("datatype not handled in tensorwrapper");
        }
    }
    if (tw)
        return tw;
    lg->error("build Tensor: not able to create the tensor from the definition:");
    lg->error("Name: '{}', dataype: '{}', dimensions: {}, size-x: {}, size-y: {}, content: '{}'",
              item.name, item.datatypeString(item.type), item.ndim, item.sizeX, item.sizeY, item.contentString(item.content));
    throw std::logic_error("Could not create a tensor.");

}

Batch *BatchManager::createBatch()
{

    Batch *b = new Batch(mBatchSize);

    // loop over tensor definition and create the required tensors....
    size_t index=0;
    for (auto &td : mTensorDef) {
        // create a tensor of the right size
        TensorWrapper *tw = buildTensor(mBatchSize, td);
        if (mBatches.size()==0)
            if (!InferenceData::checkSetup(td)) {
                lg->error("create Batch: Error:");
                lg->error("Name: '{}', dataype: '{}', dimensions: {}, size-x: {}, size-y: {}, content: '{}'",
                          td.name, td.datatypeString(td.type), td.ndim, td.sizeX, td.sizeY, td.contentString(td.content));
                throw std::logic_error("Could not create a tensor (check the logfile).");

            }

        td.index = index++; // static_cast<int>(b->mTensors.size());
        b->mTensors.push_back(tw);
    }

    // test


    return b;
}




InputTensorItem::InputTensorItem(std::string aname, std::string atype, size_t andim, size_t asizex, size_t asizey, std::string acontent)
{
    name=aname;
    type = datatypeFromString(atype);
    ndim = andim;
    sizeX = asizex;
    sizeY = asizey;
    content = contentFromString(acontent);
}

InputTensorItem::DataContent InputTensorItem::contentFromString(std::string name)
{
    if (data_contents.find(name) == data_contents.end())
        throw std::logic_error("'" + name + "' is not a valid code for data content (definition of input tensors - check the log)!");
    return data_contents[name];

}

InputTensorItem::DataType InputTensorItem::datatypeFromString(std::string name)
{
    if (data_types.find(name) == data_types.end())
        throw std::logic_error("'" + name + "' is not a valid code for a data type (definition of input tensors - check the log)!");
    return data_types[name];

}

std::string InputTensorItem::contentString(InputTensorItem::DataContent content)
{
    auto it = std::find_if(data_contents.begin(), data_contents.end(), [=](const auto &value) { return value.second == content; });
    if (it!=data_contents.end())
        return it->first;
    else
        return "invalid!";
}

std::string InputTensorItem::datatypeString(InputTensorItem::DataType dtype)
{
    auto it = std::find_if(data_types.begin(), data_types.end(), [=](const auto &value) { return value.second == dtype; });
    if (it!=data_types.end())
        return it->first;
    else
        return "invalid!";

}
