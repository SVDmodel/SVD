#include "batchmanager.h"
#include "batch.h"

#include <mutex>


#include "strtools.h"

BatchManager *BatchManager::mInstance = 0;

std::map< std::string, InputTensorItem::DataContent> data_contents = {
    {"Invalid",       InputTensorItem::Invalid},
    {"Climate",       InputTensorItem::Climate},
    {"State",         InputTensorItem::State},
    {"ResidenceTime", InputTensorItem::ResidenceTime},
    {"Neighbors",     InputTensorItem::Neighbors}
};
std::map< std::string, InputTensorItem::DataType> data_types = {
    {"Invalid", InputTensorItem::DT_INVALID},
    {"float",   InputTensorItem::DT_FLOAT},
    {"int16",   InputTensorItem::DT_INT16},
    {"int64",   InputTensorItem::DT_INT64},
    {"uint16",  InputTensorItem::DT_UINT16}
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
        spdlog::get("dnn")->debug("Batch manager created: {0:x}", (void*)this);

}

BatchManager::~BatchManager()
{
    if (spdlog::get("dnn"))
        spdlog::get("dnn")->debug("Batch manager destroyed: {0:x}", (void*)this);

    mInstance = nullptr;
}

void BatchManager::setup()
{

    lg = spdlog::get("dnn");
    if (!lg)
        throw std::logic_error("BatchManager::setup: logging not available.");
    lg->info("Setup of batch manager.");
//    mTensorDef =  {
//        {"test", InputTensorItem::DT_FLOAT, 2, 24, 10, InputTensorItem::Climate},
//        {"test2", InputTensorItem::DT_INT16, 1, 1, 0, InputTensorItem::State}
//    };
    mTensorDef =  {
        {"test", "float", 2, 24, 10, "Climate"},
        {"test2", "uint16", 1, 1, 0, "State"}
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

std::mutex batch_mutex;
std::pair<Batch *, int> BatchManager::batch()
{
    // serialize this function...
    std::lock_guard<std::mutex> guard(batch_mutex);

    if (mBatches.size()==0)
        mBatches.push_back( createBatch() );

    if (mBatches.back()->freeSlots()<=0) {
        // we need to create a new batch
        mBatches.push_back( createBatch() );
        lg->trace("created a new batch. Now the list contains {} batch(es).", mBatches.size());
    }

    Batch *last = mBatches.back();

    // get a new slot in the batch
    std::pair<Batch *, int> result;
    result.first = last;
    result.second = last->acquireSlot();
    return result;

}

TensorWrapper *BatchManager::buildTensor(int batch_size, InputTensorItem &item)
{
    TensorWrapper *tw = nullptr;
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
        }
    }
    if (item.ndim==2) {
        switch (item.type) {
            case InputTensorItem::DT_FLOAT:
            tw = new TensorWrap3d<float>(batch_size, item.sizeX, item.sizeY); break;
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
    const int batch_size=16;

    Batch *b = new Batch(batch_size);

    // loop over tensor definition and create the required tensors....
    for (auto &td : mTensorDef) {
        // create a tensor of the right size
        TensorWrapper *tw = buildTensor(batch_size, td);

        td.index = static_cast<int>(b->mTensors.size());
        b->mTensors.push_back(tw);
    }

    // test


    return b;
}




InputTensorItem::InputTensorItem(std::string aname, std::string atype, int andim, int asizex, int asizey, std::string acontent)
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
