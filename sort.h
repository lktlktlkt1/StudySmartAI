#ifndef STUDYSMART_SORT_H
#define STUDYSMART_SORT_H

#include "definition.h"

/*
 * Module 2: Sorting / Divide-and-Conquer
 *
 * This module ranks study tasks using Merge Sort.
 * The ranking order is definedby ss_task_priority_cmp().
 */

/*
 * Merges two sorted ranges of tasks into one sorted range.
 *
 * The left range is tasks[left..mid].
 * The right range is tasks[mid + 1..right].
 *
 * The merged result is first stored in temp[] and then copied back into tasks[left..right].
 */
static void ss_sort_merge(Task tasks[], Task temp[], int left, int mid,
                          int right)
{
  int i = left;
  int j = mid + 1;
  int k = left;

  /* Compare the front task of each sorted half and copy the one that should appear earlier in the ranking. */
  while (i <= mid && j <= right)
  {
    if (ss_task_priority_cmp(&tasks[i], &tasks[j]) <= 0)
    {
      temp[k++] = tasks[i++];
    }
    else
    {
      temp[k++] = tasks[j++];
    }
  }

  /* Copy any remaining tasks from the left half. */
  while (i <= mid)
  {
    temp[k++] = tasks[i++];
  }

  /* Copy any remaining tasks from the right half. */
  while (j <= right)
  {
    temp[k++] = tasks[j++];
  }

  /* Move the merged result back into the original task array. */
  for (i = left; i <= right; i++)
  {
    tasks[i] = temp[i];
  }
}

/*
 * Recursively sorts a range of tasks using Merge Sort.
 *
 * Divide: the task range is split into two smaller halves until the range contains only one task.
 * Conquer: each half is sorted recursively.
 * Combine: the two sorted halves are merged back together until the whole range becomes sorted.
 *
 */
static void ss_sort_merge_sort_range(Task tasks[], Task temp[], int left,
                                     int right)
{
  if (left >= right)
  {
    return;
  }

  int mid = left + (right - left) / 2;
  ss_sort_merge_sort_range(tasks, temp, left, mid);
  ss_sort_merge_sort_range(tasks, temp, mid + 1, right);
  ss_sort_merge(tasks, temp, left, mid, right);
}

/*
 * Runs the sorting strategy for one scenario.
 *
 * The scenario tasks are copied to result->orderedTasks and sorted using Merge Sort.
 *
 * After sorting, tasks are selected from the sorted order until adding the next task would exceed the available study time.
 */
static void ss_sort_run(const Scenario *scenario, AlgorithmResult *result)
{
  ss_result_init(result, STRATEGY_SORTING);
  if (scenario == NULL || result == NULL)
  {
    return;
  }

  /* Copy the original scenario tasks */
  result->orderedCount = scenario->taskCount;
  for (int i = 0; i < scenario->taskCount; i++)
  {
    result->orderedTasks[i] = scenario->tasks[i];
  }

  /* Rank the copied tasks using Merge Sort. */
  if (result->orderedCount > 1)
  {
    Task temp[SS_MAX_TASKS];
    ss_sort_merge_sort_range(result->orderedTasks, temp, 0,
                             result->orderedCount - 1);
  }

  /*
   * The sorting module primarily ranks tasks.
   * For the comparison table, it uses the ranked prefix until the next task would exceed the time budget.
   */
  for (int i = 0; i < result->orderedCount; i++)
  {
    Task *task = &result->orderedTasks[i];
    if (result->totalStudyTime + task->studyTime > scenario->availableTime)
    {
      break;
    }
    ss_result_add_selected(result, task);
  }
}

#endif
