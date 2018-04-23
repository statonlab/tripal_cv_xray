<?php

class XRayDispatcherJob implements XRayJob {

  /**
   * Chunk size.
   *
   * @var int
   */
  protected $chunk;

  /**
   * The CV short names that we will index.
   *
   * @var array
   */
  protected $cv_shortnames;

  /**
   * The bundle IDs that we will index.  Please note these are the entities that will be mapped onto the anchor.  For example, these are the genes that will be mapped onto organism.
   *
   * @var
   */
  protected $bundle_ids;


  /**
   * XRayDispatcherJob constructor.
   *
   * Create a new dispatcher job.
   *
   * @param array[int] $bundle_ids
   * @param array[string] $cv_shortnames
   * @param int $chunk_size
   */
  public function __construct($bundle_ids, $cv_shortnames, $chunk_size = 100) {
    $this->chunk = $chunk_size;
    $this->bundle_ids = $bundle_ids;
    $this->cv_shortnames = $cv_shortnames;

  }

  /**
   * Creates indexer jobs per bundle as chunks of entities.
   */
  public function handle() {
    $this->clearIndexTable();

    $bundles = $this->bundles();
    foreach ($bundles as $bundle) {
      $total = $this->bundleTotal($bundle);
      for ($i = 0; $i < $total; $i += $this->chunk) {
        $job = new XRayIndexerJob($bundle, $this->cv_shortnames, TRUE);
        $job->offset($i)->limit($this->chunk);
        XRayQueue::dispatch($job);
      }
    }
  }

  /**
   * Get the total number of entities in a bundle.
   *
   * @param $bundle
   *
   * @return int
   */
  public function bundleTotal($bundle) {
    $bundle_table = "chado_bio_data_{$bundle->bundle_id}";
    return (int) db_select($bundle_table)
      ->countQuery()
      ->execute()
      ->fetchField();
  }

  /**
   * Get the bundles we are interested in.
   *
   * @return mixed
   */
  public function bundles() {
    $query = db_select('chado_bundle', 'CB');
    $query->fields('CB', ['bundle_id', 'data_table']);
    $query->fields('TB', ['label']);
    $query->join('tripal_bundle', 'TB', 'TB.id = CB.bundle_id');
    $query->condition('bundle_id', $this->bundle_ids, 'IN');
    return $query->execute()->fetchAll();
  }

  /**
   * Truncate the tripal_cvterm_entity_linker table.
   *
   * @return \DatabaseStatementInterface
   */
  public function clearIndexTable() {
    return db_truncate('tripal_cvterm_entity_linker')->execute();
  }
}
