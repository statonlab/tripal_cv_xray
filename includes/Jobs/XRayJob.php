<?php

/**
 * Defines an interface for any XRay jobs.
 *
 * All jobs must implement this interface.
 */
interface XRayJob {
  public function handle();
}
