#ifndef STUDYSMART_AI_H
#define STUDYSMART_AI_H

#include "definition.h"

/* Extract compact numeric features used by the recommendation rules. */
static ScenarioFeatures ss_ai_extract_features(const Scenario *scenario) {
  ScenarioFeatures features;
  memset(&features, 0, sizeof(features));

  /* Empty input returns all-zero features. */
  if (scenario == NULL || scenario->taskCount <= 0) {
    return features;
  }

  int totalImportance = 0;
  int urgentCount = 0;
  int minImportance = scenario->tasks[0].importance;
  int maxImportance = scenario->tasks[0].importance;

  features.numTasks = scenario->taskCount;
  features.availableTime =
      scenario->availableTime > 0 ? scenario->availableTime : 1;

  for (int i = 0; i < scenario->taskCount; i++) {
    const Task *task = &scenario->tasks[i];
    features.totalRequiredTime += task->studyTime;
    totalImportance += task->importance;

    /* A task due within 3 days is treated as urgent to calculate Deadline Tightness.*/
    if (task->deadline <= 3) {
      urgentCount++;
    }
    if (task->importance < minImportance) {
      minImportance = task->importance;
    }
    if (task->importance > maxImportance) {
      maxImportance = task->importance;
    }
  }

  /* Store ratios as percentages to keep the AI module integer-only. */
  features.timePressureRatio =
      (features.totalRequiredTime * 100) / features.availableTime;
  features.averageImportance =
      (totalImportance + features.numTasks / 2) / features.numTasks;
  features.deadlineTightness = (urgentCount * 100) / features.numTasks;
  features.importanceVariation = maxImportance - minImportance;

  return features;
}

/* Predict the planning strategy with a small rule-based decision tree. */
static StrategyType ss_ai_predict_strategy(const ScenarioFeatures *features) {
  if (features == NULL) {
    return STRATEGY_DP;
  }

  /*
   * Decision tree (2 features, 2 levels):
   * - Time pressure < 150%  -> MergeSort.
   * - Pressure >= 150% and tight deadlines  -> Greedy.
   * - Pressure >= 150% and relaxed deadlines -> DP.
   */
  if (features->timePressureRatio < 150) {
    return STRATEGY_SORTING;
  }
  if (features->deadlineTightness >= 75) {
    return STRATEGY_GREEDY;
  }
  return STRATEGY_DP;
}

#endif
