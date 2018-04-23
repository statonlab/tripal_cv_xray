<?php

class XRayDispatcherJob implements XRayJob {

  /**
   * Supported chado tables.
   *
   * @var array
   */
  protected $supportedTables = ['feature'];

  /**
   * Chunk size.
   *
   * @var int
   */
  protected $chunk;

  /**
   * XRayDispatcherJob constructor.
   *
   * Create a new dispatcher job.
   *
   * @param int $chunk_size
   */
  public function __construct($chunk_size = 100) {
    $this->chunk = $chunk_size;
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
        $job = new XRayIndexerJob($bundle, TRUE);
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
    $query->condition('data_table', $this->supportedTables, 'IN');
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
