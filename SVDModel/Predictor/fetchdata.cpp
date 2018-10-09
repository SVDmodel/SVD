#include "fetchdata.h"

#include <regex>

#include "model.h"
#include "tensorhelper.h"
#include "batchdnn.h"
#include "settings.h"
#include "expressionwrapper.h"

void FetchData::setup(const Settings * /*settings*/, const std::string & /*key*/, const InputTensorItem & /*item*/)
{

}

void FetchData::fetch(Cell * /* cell */, BatchDNN* /*batch */, size_t /* slot */)
{
}

FetchData *FetchData::createFetchObject(InputTensorItem *def)
{
    FetchData *f=nullptr;
    switch (def->content) {
    case InputTensorItem::Climate:
    case InputTensorItem::State:
    case InputTensorItem::ResidenceTime:
    case InputTensorItem::SiteNPKA:
    case InputTensorItem::Scalar:
    case InputTensorItem::DistanceOutside:
    case InputTensorItem::Neighbors:
        f = new FetchDataStandard(def);
        break;
    case InputTensorItem::Variable:
        f = new FetchDataVars(def);
        break;
    default:
        throw std::logic_error("Unknown type of data extractor for: " + def->name);
    }

    return f;
}

void FetchDataStandard::setup(const Settings * /*settings*/, const std::string & /*key*/, const InputTensorItem & /*item*/)
{

}

void FetchDataStandard::fetch(Cell *cell, BatchDNN *batch, size_t slot)
{
    switch (mItem->content) {
    case InputTensorItem::Climate:
        fetchClimate(cell, batch, slot);
        break;
    case InputTensorItem::State:
        fetchState(cell, batch, slot);
        break;
    case InputTensorItem::ResidenceTime:
        fetchResidenceTime(cell, batch, slot);
        break;
    case InputTensorItem::SiteNPKA:
        fetchSite(cell, batch, slot);
        break;
    case InputTensorItem::Neighbors:
        fetchNeighbors(cell, batch, slot);
        break;
    case InputTensorItem::Scalar:
        // a scalar is already set to the correct value.
        break;
    case InputTensorItem::DistanceOutside:
        fetchDistanceOutside(cell, batch, slot);
        break;

    default:
        throw std::logic_error("InferenceData::fetchData: invalid content type.");

    }

}



void FetchDataStandard::fetchClimate(Cell *cell, BatchDNN* batch, size_t slot)
{

    // the climate data
    const auto &ec = cell->environment();
    auto climate_series = Model::instance()->climate()->series(Model::instance()->year(),
                                                               mItem->sizeX,
                                                               ec->climateId());
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap3d<float> *tw = static_cast<TensorWrap3d<float>*>(t);

    if (climate_series.size() != mItem->sizeX || climate_series[0]->size() != mItem->sizeY)
        throw std::logic_error("FetchDataStandard::fetchClimate: mismatch in dimensions: expected " +
                               to_string(mItem->sizeX) + ", got " + to_string(climate_series.size()) +  " years; " +
                               "expected " + to_string(mItem->sizeY) + ", got " + to_string(climate_series[0]->size()) + " columns (per year)!");
    //return;
    // copy the climate data to the tensors
    // TODO: transform inputs
    size_t i = 0;
    for (const std::vector<float> *p : climate_series) {
        float *d = tw->row(slot, i++);
        memcpy(d, p->data(), sizeof(float) * p->size());
    }

}

void FetchDataStandard::fetchState(Cell *cell, BatchDNN* batch, size_t slot)
{
    // the current state
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap2d<short int> *tw = static_cast<TensorWrap2d<short int>*>(t);
    short int *p = tw->example(slot);
    // stateId starts with 1, the state tensor is 0-based
    *p = cell->stateId() - 1;

}

void FetchDataStandard::fetchResidenceTime(Cell *cell, BatchDNN* batch, size_t slot)
{
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(slot);
    // TODO: residence time, now fixed divide by 10
    *p = static_cast<float>(cell->residenceTime() / 10.f);
}

void FetchDataStandard::fetchNeighbors(Cell *cell, BatchDNN* batch, size_t slot)
{
    const size_t n_neighbors = 62; // 2x32
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(slot);

    auto neighbors = cell->neighborSpecies();
    if (neighbors.size() != n_neighbors)
        throw std::logic_error("Invalid number of neighbors...");

    for (size_t i=0;i<n_neighbors;++i)
        *p++ = static_cast<float>(neighbors[i]);

}

void FetchDataStandard::fetchSite(Cell *cell, BatchDNN* batch, size_t slot)
{
    const int i_nitrogen=0, i_soildepth=1;
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(slot);
    // site: nitrogen/soil-depth
    const auto &ec = cell->environment();
    // TODO: transformation...
    *p++ = static_cast<float>( (ec->value(i_nitrogen) -58.500)/41.536 );
    *p++ = static_cast<float>( (ec->value(i_soildepth)-58.500)/41.536 );
}

void FetchDataStandard::fetchDistanceOutside(Cell *cell, BatchDNN* batch, size_t slot)
{
    const int i_distance = 2;
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(slot);
    const auto &ec = cell->environment();

    *p = static_cast<float>( ec->value(i_distance) );

}

void FetchDataVars::setup(const Settings *settings, const std::string &key, const InputTensorItem &item)
{
    // expects as many expressions as 'sizeX', one-dimensional, and datatype float
    auto lg = spdlog::get("dnn");
    std::string tlist = settings->valueString(key + ".transformations");
    lg->debug("Set up expressions: {}", tlist);
    bool has_error = false;
    try {
      std::regex re("\\{([^\\}]*)\\}");
      std::sregex_iterator next(tlist.begin(), tlist.end(), re);
      std::sregex_iterator end;
      while (next != end) {
        std::smatch match = *next;
        lg->debug("expression: {}", match.str(1));

        mExpressions.push_back(new Expression(match.str(1)));
        next++;
      }
      if (item.type != InputTensorItem::DT_FLOAT) {
          lg->error("Setup of Tensor {} (type: {}): Datatype 'float' expected.", item.name, item.contentString(item.content));
          has_error = true;
      }
      if (item.ndim != 1 || item.sizeX != mExpressions.size()) {
          lg->error("Setup of Tensor {} (type: {}): Number of dimensions wrong (expected: 1), or number of expressions ({}) does not match the size of the tensor ({}).", item.name,
                    item.contentString(item.content),
                    mExpressions.size(), item.sizeX);
          has_error = true;
      }
      if (has_error)
          throw std::logic_error("Error in setting up an Input tensor. Check the log.");

    } catch (std::regex_error& e) {
      throw std::logic_error(to_string("Error in setting up expression (regexp error): ") + e.what());
    }
}

void FetchDataVars::fetch(Cell *cell, BatchDNN *batch, size_t slot)
{
    TensorWrapper *t = batch->tensor(mItem->index);
    TensorWrap2d<float> *tw = static_cast<TensorWrap2d<float>*>(t);
    float *p = tw->example(slot);
    CellWrapper cw(cell);
    for (auto &expr : mExpressions) {
        *p++ = static_cast<float>( expr->calculate(cw) );
    }

}
