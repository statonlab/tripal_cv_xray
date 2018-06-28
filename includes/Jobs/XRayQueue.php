<?php

class XRayQueue {

  /**
   * The callback for cron queue module to call.
   *
   * @param \XRayJob $job
   *
   * @throws \Exception
   */
  public static function run(XRayJob $job) {
    try {
      $job->handle();
    } catch (Exception $exception) {
      tripal_report_error('tripal_cv_xray', TRIPAL_ERROR, $exception->getMessage());
    }
  }

  /**
   * Add a job to the queue.
   *
   * @param \XRayJob $job
   *
   * @return mixed
   */
  public static function dispatch(XRayJob $job) {
    return DrupalQueue::get('tripal_cv_xray')->createItem($job);
  }
}
