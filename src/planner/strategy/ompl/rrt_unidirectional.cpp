#include "rrt_unidirectional.h"
#include "elements/plannerdata_vertex_annotated.h"
#include "planner/validitychecker/validity_checker_ompl.h"

using namespace ompl::geometric;

RRTUnidirectional::RRTUnidirectional(const base::SpaceInformationPtr &si, Quotient *previous ): og::Quotient(si, previous)
{
  deltaCoverPenetration_ = 0.05;
  goalBias_ = 0.01;
  totalNumberOfSamples = 0;
  graphLength = 0.0;
}

RRTUnidirectional::~RRTUnidirectional(void)
{
}

void RRTUnidirectional::Sample(Configuration *q_random){
  totalNumberOfSamples++;
  if(previous == nullptr){
    if(!hasSolution && rng_.uniform01() < goalBias_){
      goal->sampleGoal(q_random->state);
    }else{
      sampler_->sampleUniform(q_random->state);
    }
  }else{
    if(!hasSolution && rng_.uniform01() < goalBias_){
      goal->sampleGoal(q_random->state);
    }else{
      ob::SpaceInformationPtr M0 = previous->getSpaceInformation();
      base::State *s_C1 = C1->allocState();
      base::State *s_M0 = M0->allocState();

      C1_sampler->sampleUniform(s_C1);
      previous->SampleGraph(s_M0);
      mergeStates(s_M0, s_C1, q_random->state);

      C1->freeState(s_C1);
      M0->freeState(s_M0);
    }
  }
}

void RRTUnidirectional::setRange(double distance)
{
  maxDistance_ = distance;
}

double RRTUnidirectional::getRange() const
{
  return maxDistance_;
}

uint RRTUnidirectional::GetNumberOfVertices()
{
  return G_->size();
}

uint RRTUnidirectional::GetNumberOfEdges()
{
  std::vector<Configuration *> configs;
  if (G_){
    G_->list(configs);
  }
  uint ctr_edges = 0;
  for (auto &config : configs)
  {
    if (config->parent != nullptr)
    {
      ctr_edges++;
    }
  }
  return ctr_edges;
}


RRTUnidirectional::Configuration* RRTUnidirectional::Nearest(Configuration *q_random)
{
  return G_->nearest(q_random);
}

RRTUnidirectional::Configuration* RRTUnidirectional::Connect(Configuration *q_near, Configuration *q_random)
{
  //##############################################################################
  // q_new_state <- BALL_maxDistance_(q_near) in direction of q_random
  //##############################################################################
  double d = si_->distance(q_near->state, q_random->state);
  //double maxD = q_near->GetRadius();
  double maxD = maxDistance_;
  if(d > maxD){
    //maxD/d -> t \in [0,1] on boundary
    si_->getStateSpace()->interpolate(q_near->state, q_random->state, maxD / d, q_random->state);
  }

  //##############################################################################
  // extend the tree from q_near towards q_new
  //##############################################################################

  if(si_->checkMotion(q_near->state, q_random->state)){
    auto *q_new = new Configuration(si_);
    si_->copyState(q_new->state, q_random->state);
    q_new->parent = q_near;
    q_new->parent_edge_weight = si_->distance(q_near->state, q_new->state);
    graphLength += q_new->parent_edge_weight + q_new->parent_edge_weight*epsilon*si_->getSpaceMeasure();

    // auto checkerPtr = static_pointer_cast<OMPLValidityChecker>(si_->getStateValidityChecker());
    // double d1 = checkerPtr->Distance(q_new->state);
    // q_new->openset = new cover::OpenSetHypersphere(si_, q_new->state, d1);

    G_->add(q_new);

    return q_new;
  }
  return nullptr;
}

double RRTUnidirectional::GetSamplingDensity(){
  // if(previous==nullptr){
  //   return (double)GetNumberOfVertices()/((double)si_->getSpaceMeasure());
  // }else{
  //   return (double)GetNumberOfVertices()/(previous->GetGraphLength()*C1->getSpaceMeasure());
  // }
  if(previous==nullptr){
    return (double)totalNumberOfSamples/((double)si_->getSpaceMeasure());
  }else{
    return (double)totalNumberOfSamples/(previous->GetGraphLength()*C1->getSpaceMeasure());
  }
}

bool RRTUnidirectional::ConnectedToGoal(Configuration* q)
{
  double dist=0.0;
  if(q != nullptr){
    if(goal->isSatisfied(q->state, &dist)){
      hasSolution = true;
      return true;
    }
  }
  return false;
}

void RRTUnidirectional::ConstructSolution(Configuration *configuration_goal)
{
  if (configuration_goal != nullptr){

    std::vector<Configuration *> q_path;
    while (configuration_goal != nullptr){
      q_path.push_back(configuration_goal);
      configuration_goal = configuration_goal->parent;
    }

    auto path(std::make_shared<PathGeometric>(si_));
    for (int i = q_path.size() - 1; i >= 0; --i){
      path->append(q_path[i]->state);
    }
    pdef_->addSolutionPath(path);
  }
}

void RRTUnidirectional::Init()
{
  //setRange(0.1);

  //std::cout << "Planner " + getName() + " specs:" << std::endl;
  //std::cout << "Multithreaded:                 " << (getSpecs().multithreaded ? "Yes" : "No") << std::endl;
  //std::cout << "Reports approximate solutions: " << (getSpecs().approximateSolutions ? "Yes" : "No") << std::endl;
  //std::cout << "Can optimize solutions:        " << (getSpecs().optimizingPaths ? "Yes" : "No") << std::endl;
  //std::cout << "Range:                         " << getRange() << std::endl;
  //std::cout << "Aware of the following parameters:";
  //std::vector<std::string> params;
  //params_.getParamNames(params);
  //for (auto &param : params)
  //    std::cout << " " << param;
  //std::cout << std::endl;

  checkValidity();
  goal = dynamic_cast<ob::GoalSampleableRegion *>(pdef_->getGoal().get());

  while (const ob::State *st = pis_.nextStart()){
    q_start = new Configuration(si_);
    si_->copyState(q_start->state, st);

    // auto checkerPtr = static_pointer_cast<OMPLValidityChecker>(si_->getStateValidityChecker());
    // double d1 = checkerPtr->Distance(q_start->state);
    // q_start->openset = new cover::OpenSetHypersphere(si_, q_start->state, d1);

    G_->add(q_start);
  }

  if (const ob::State *st = pis_.nextGoal()){
    q_goal = new Configuration(si_);
    si_->copyState(q_goal->state, st);

    // auto checkerPtr = static_pointer_cast<OMPLValidityChecker>(si_->getStateValidityChecker());
    // double d1 = checkerPtr->Distance(q_goal->state);
    // q_goal->openset = new cover::OpenSetHypersphere(si_, q_goal->state, d1);
  }else{
    OMPL_ERROR("%s: There is no valid goal state!", getName().c_str());
    exit(0);
  }


  if (G_->size() == 0){
    OMPL_ERROR("%s: There are no valid initial states!", getName().c_str());
    exit(0);
  }
  if (!sampler_) sampler_ = si_->allocStateSampler();
}


void RRTUnidirectional::clear()
{
  Planner::clear();
  sampler_.reset();
  freeMemory();
  if(G_){
    G_->clear();
  }
  hasSolution = false;
  lastExtendedConfiguration = nullptr;
  if(q_goal != nullptr){
    if(q_goal->state!=nullptr){
      si_->freeState(q_goal->state);
    }
    delete q_goal;
  }
  q_start = nullptr;
  q_goal = nullptr;

  pis_.restart();
}

void RRTUnidirectional::freeMemory()
{
  if (G_)
  {
    std::vector<Configuration *> configurations;
    G_->list(configurations);
    for (auto &configuration : configurations)
    {
      if (configuration->state != nullptr)
      {
        si_->freeState(configuration->state);
      }
      delete configuration;
    }
  }
}

void RRTUnidirectional::setup(void)
{
  Planner::setup();
  tools::SelfConfig sc(si_, getName());
  sc.configurePlannerRange(maxDistance_);

  if (!G_){
    G_.reset(tools::SelfConfig::getDefaultNearestNeighbors<Configuration *>(this));
  }
  G_->setDistanceFunction([this](const Configuration *a, const Configuration *b)
                           {
                           //distance between surrounding hyperspheres. if
                           //overlapping, return 0
                              //double ra = a->GetRadius();
                              //double rb = b->GetRadius();
                              //return max( si_->distance(a->state, b->state)-ra-rb, 0.0);
                              return si_->distance(a->state, b->state);
                           });

}

void RRTUnidirectional::getPlannerData(base::PlannerData &data) const
{
  std::vector<Configuration *> vertices;

  if (G_){
    G_->list(vertices);
  }

  if (lastExtendedConfiguration != nullptr){
    data.addGoalVertex(PlannerDataVertexAnnotated(lastExtendedConfiguration->state, 0, lastExtendedConfiguration->GetRadius()));
  }

  for (auto &vertex : vertices)
  {
    double d = vertex->GetRadius();
    if (vertex->parent == nullptr){
      data.addStartVertex(PlannerDataVertexAnnotated(vertex->state, 0, d));
    }else{
      double dp = vertex->parent->GetRadius();
      data.addEdge(PlannerDataVertexAnnotated(vertex->parent->state, 0, dp), PlannerDataVertexAnnotated(vertex->state, 0, d));
    }
    if(!vertex->state){
      std::cout << "vertex state does not exists" << std::endl;
      si_->printState(vertex->state);
      exit(0);
    }
  }
}

void RRTUnidirectional::Grow(double t)
{
  Configuration *q_random = new Configuration(si_);

  Sample(q_random);

  Configuration *q_near = Nearest(q_random);
  Configuration *q_new = Connect(q_near, q_random);

  if(q_new != nullptr){
    lastExtendedConfiguration = q_new;
  }
  if(q_random->state != nullptr){
    si_->freeState(q_random->state);
  }
  delete q_random;
}


ob::PlannerStatus RRTUnidirectional::solve(const ob::PlannerTerminationCondition &ptc)
{
  Init();

  while(!ptc){

    Grow();

    if(HasSolution())
    {
      ConstructSolution(lastExtendedConfiguration);
      return ob::PlannerStatus::EXACT_SOLUTION;
    }
  }

  OMPL_INFORM("%s: Created %u states", getName().c_str(), G_->size());
  return ob::PlannerStatus::TIMEOUT;
}

bool RRTUnidirectional::HasSolution()
{
  if(!hasSolution){
    if(ConnectedToGoal(lastExtendedConfiguration)){
      hasSolution = true;
    }
  }
  return hasSolution;
}

void RRTUnidirectional::CheckForSolution(ob::PathPtr &solution)
{
  if(!hasSolution) return;

  Configuration *configuration_goal = lastExtendedConfiguration;
  if (configuration_goal != nullptr){

    std::vector<Configuration *> q_path;
    while (configuration_goal != nullptr){
      q_path.push_back(configuration_goal);
      configuration_goal = configuration_goal->parent;
    }

    auto path(std::make_shared<PathGeometric>(si_));
    for (int i = q_path.size() - 1; i >= 0; --i){
      path->append(q_path[i]->state);
    }
    solution = path;
    goalBias_ = 0.0;
  }
}
bool RRTUnidirectional::SampleGraph(ob::State *q_random_graph)
{
  PDF<Configuration*> pdf = GetConfigurationPDF();

  Configuration *q = pdf.sample(rng_.uniform01());
  double t = rng_.uniform01();

  const ob::State *q_from = q->state;
  const ob::State *q_to = q->parent->state;
  M1->getStateSpace()->interpolate(q_from, q_to, t, q_random_graph);

  //std::cout << maxDistance_ << std::endl;
  //sampler_->sampleGaussian(q_random_graph, q_random_graph, epsilon);
  sampler_->sampleUniformNear(q_random_graph, q_random_graph, epsilon);
  return true;
}

ompl::PDF<RRTUnidirectional::Configuration*> RRTUnidirectional::GetConfigurationPDF()
{
  PDF<Configuration*> pdf;
  std::vector<Configuration *> configurations;
  if(G_){
    G_->list(configurations);
  }
  for (auto &configuration : configurations)
  {
    if(!(configuration->parent == nullptr))
    {
      //pdf.add(configuration, 1.0/configuration->GetRadius());
      //pdf.add(configuration, exp(-configuration->GetRadius()));
      //pdf.add(configuration, d);
      //double d = configuration->GetRadius();
      //pdf.add(configuration, 1.0);
      pdf.add(configuration, configuration->parent_edge_weight);
      //pdf.add(configuration, 1.0/d);
      //pdf.add(configuration, exp(-d));
    }
  }

  return pdf;
}
