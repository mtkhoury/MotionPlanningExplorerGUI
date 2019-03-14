from xml.dom.minidom import parse
from xml.etree import ElementTree as ET
import os
import sys
import numpy as np

class PlannerResults:
  name = ""
  run_ctr = 0
  number_of_levels = 0
  dimensions_per_level = []

  run_nodes_per_level = []
  run_feasible_nodes_per_level = []
  run_time = []

  def __init__(self, xml, runcount):
    self.dimensions_per_level = []
    for child in xml:
      if child.tag == "name":
        self.name = child.text
      if child.tag == "number_of_levels":
        self.number_of_levels = int(child.text)
      if child.tag == "levels":
        for d in child:
          self.dimensions_per_level.append(int(d.text))
        self.dimensions_per_level = np.array(self.dimensions_per_level).flatten()

    self.runcount = runcount
    self.levels = len(self.dimensions_per_level)

    self.run_nodes_per_level = np.zeros((self.runcount, self.levels),dtype=np.int)
    self.run_feasible_nodes_per_level = np.zeros((self.runcount, self.levels),dtype=np.int)
    self.run_time = np.zeros(self.runcount)

    self.AddRuns(xml)

  def GetLargestDimension(self):
    return self.dimensions_per_level[-1]

  def AverageTime(self):
    t =  np.mean(self.run_time)
    return t

  def AddRuns(self, xml):
    for child in xml:
      if child.tag == "run":
        run_ctr = int(child.attrib.get('number'))-1
        for rchild in child:
          if rchild.tag == "time":
            self.run_time[run_ctr] = float(rchild.text)
          if rchild.tag == "levels":
            for level in rchild:
              level_ctr = 0
              for lchild in level:
                if lchild.tag == "nodes":
                  self.run_nodes_per_level[run_ctr, level_ctr] = float(lchild.text)
                if lchild.tag == "feasible_nodes":
                  self.run_feasible_nodes_per_level[run_ctr, level_ctr] = float(lchild.text)


  def __repr__(self):
    rstr = self.name + ": " + str(self.dimensions_per_level)
    rstr = rstr + "Runs: " + str(self.runcount)
    rstr = rstr + str(self.run_time)
    rstr = rstr + "\n------------\n"
    return rstr

  def __str__(self):
      return self.__repr__()

class BenchmarkAnalytica:
  fname = ""
  fname_pdf = ""
  benchmark_name = ""
  planners = []
  timelimit = 0
  runcount = 0
  number_of_levels = 0
  number_of_planners = 0
  dimensions_per_level = []

  def __repr__(self):
    rstr = ""
    rstr = rstr + "planners:" + str(self.nr_planners) 
    rstr = rstr + " runs:" + str(self.runcount) + "\n"
    rstr = rstr + str(self.dimensions_per_level) + "\n"
    for p in self.planners:
      rstr = rstr + str(p) + "\n"
    return rstr

  def __str__(self):
      return self.__repr__()

  def DimensionToIndex(self, d):
    return np.where(self.dimensions_per_level==d)[0][0]

  def AddPlanner(self, presult ):
    self.planners.append(presult)

  def __init__(self, fname = "../../data/benchmarks/last.xml"):
    self.planners = []
    self.fname = fname
    fname_dir = os.path.dirname(fname)
    self.fname_pdf = fname_dir + "/" + fname + "_runtimes.pdf"

    xml = ET.parse(self.fname)
    benchmark = xml.getroot()

    for child in benchmark:
      tag = child.tag
      if tag == "name":
        self.benchmark_name = child.text
      elif tag == "number_levels":
        self.number_of_levels = int(child.text)
      elif tag == "number_of_planners":
        self.number_of_planners = int(child.text)
      elif tag == "run_count":
        self.runcount = int(child.text)
      elif tag == "max_time":
        self.timelimit = float(child.text)
      elif tag == "levels":
        for d in child:
          self.dimensions_per_level.append(int(d.text))
        self.dimensions_per_level = np.array(self.dimensions_per_level)

    for child in benchmark:
      if child.tag == "planner":
        self.AddPlanner(PlannerResults(child, self.runcount))

  ###ANALYTICS
  def PlannerNames(self):
    names = []
    for planner in self.planners:
      names.append(planner.name)
    return names

  def AverageTimePerPlanner(self):
    time = []
    for planner in self.planners:
      time.append(planner.AverageTime())
    time = np.array(time)
    return time

  def AverageTimePerPlannerSingleSubspace(self):
    time = []
    for planner in self.planners:
      if planner.number_of_levels == 1:
        time.append(planner.AverageTime())
    time = np.array(time)
    return time

  def GetAverageTimePerPlannerPerDimensionalityMatrix(self):
    planners_per_dim = np.zeros((self.number_of_levels, self.number_of_planners))
    pctr=0
    for planner in self.planners:
      idx = self.DimensionToIndex(planner.GetLargestDimension())
      planners_per_dim[idx,pctr]=planner.AverageTime()
      pctr=pctr+1
    planners_per_dim = np.where(planners_per_dim!=0,planners_per_dim,np.nan)
    return planners_per_dim

  def TimeAveragePerDimensionality(self):
    P = self.GetAverageTimePerPlannerPerDimensionalityMatrix()
    return np.nanmean(P, axis=1)

  def TimeMaxPerDimensionality(self):
    P = self.GetAverageTimePerPlannerPerDimensionalityMatrix()
    return np.nanmax(P, axis=1)

  def TimeMinPerDimensionality(self):
    P = self.GetAverageTimePerPlannerPerDimensionalityMatrix()
    return np.nanmin(P, axis=1)

  def TimeAveragePerSingleDimensionalityAlgorithm(self):
    planners_per_dim = np.zeros(self.number_of_levels)
    pctr=0
    for planner in self.planners:
      if planner.number_of_levels == 1:
        idx = self.DimensionToIndex(planner.GetLargestDimension())
        planners_per_dim[idx]=planner.AverageTime()
    planners_per_dim = np.where(planners_per_dim!=0,planners_per_dim,np.nan)
    return planners_per_dim

  def GroupPlannersByDimensionality(self):
    planners_per_dim = np.zeros((self.number_of_levels, self.number_of_planners))
    pctr=0
    for planner in self.planners:
      idx = self.DimensionToIndex(planner.GetLargestDimension())
      planners_per_dim[idx,pctr]=planner.AverageTime()
      pctr=pctr+1
    print planners_per_dim



if __name__ == '__main__':
  fname = "../../data/benchmarks/last.xml"
  benchmark = BenchmarkAnalytica(fname)
  print benchmark.AverageTimePerPlanner()
  print benchmark.PlannerNames()
  print benchmark.AverageTimePerDimensionality()
