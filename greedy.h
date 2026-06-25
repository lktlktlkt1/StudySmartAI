#ifndef STUDYSMART_GREEDY_H
#define STUDYSMART_GREEDY_H

#include "definition.h"

// scan all tasks and return the best one we can still take
// conditions: not used yet + won't exceed total available time
static int ss_greedy_find_best_feasible(const Scenario *scenario,
                                        const bool used[], int usedTime) {
  int bestIndex = -1;  // track current best candidate

  for (int i = 0; i < scenario->taskCount; i++) {

    if (used[i]) {
      continue;  // skip tasks already selected
    }

    // skip if adding this task would break the time limit
    if (usedTime + scenario->tasks[i].studyTime > scenario->availableTime) {
      continue;
    }

    // update if: no candidate yet, orcurrent task has higher priority (cmp < 0 means better)
    if (bestIndex < 0 ||
        ss_task_priority_cmp(&scenario->tasks[i], &scenario->tasks[bestIndex]) < 0) {
      bestIndex = i;
    }
  }

  return bestIndex;  // -1 -> no feasible task left
}


// greedy loop: keep taking the best feasible task each round
// stops when no more task can fit into remaining time
static void ss_greedy_run(const Scenario *scenario, AlgorithmResult *result) {

  ss_result_init(result, STRATEGY_GREEDY);

  if (scenario == NULL || result == NULL) {
    return;
  }

  bool used[SS_MAX_TASKS] = {false};  // mark selected tasks

  while (true) {

    int bestIndex =
        ss_greedy_find_best_feasible(scenario, used, result->totalStudyTime);

    if (bestIndex < 0) {
      break;  // nothing else can be added
    }

    used[bestIndex] = true;

    // append to result in selection order
    result->orderedTasks[result->orderedCount++] = scenario->tasks[bestIndex];

    // update total time / score inside result
    ss_result_add_selected(result, &scenario->tasks[bestIndex]);
  }
}

#endif
