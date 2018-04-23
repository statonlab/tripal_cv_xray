<?php

class XRayIndexerJob implements XRayJob {

  /**
   * Chunk size.
   *
   * @var int
   */
  protected $chunk = 100;

  /**
   * Starting position.
   *
   * @var int
   */
  protected $offset = 0;

  /**
   * Primary keys cache.
   *
   * @var array
   */
  protected static $primaryKeys = [];

  /**
   * Verbose output or silent.
   *
   * @var bool
   */
  protected $verbose;

  /**
   * The bundle to index.
   *
   * @var object
   */
  protected $bundle;


  /**
   * The CV shortnames to index.
   * @var array
   */
  protected $cv_shortnames;

  /**
   * Create a new indexing job.
   *
   * @param object $bundle
   * @param array $cv_shortnames
   * @param bool $verbose
   */
  public function __construct($bundle, $cv_shortnames, $verbose = FALSE) {
    $this->bundle = $bundle;
  }

  /**
   * Starting position.
   *
   * @param int $offset
   */
  public function offset($offset) {
    $this->offset = $offset;
    return $this;
  }

  /**
   * Limit.
   *
   * @param $limit
   */
  public function limit($limit) {
    $this->chunk = $limit;
    return $this;
  }

  /**
   * Start the indexing job.
   *
   * @throws \Exception
   */
  public function handle() {
    $entities = $this->getEntitiesChunk();
    $data = $this->loadData($entities);
    $this->insertData($data);

    if ($this->verbose) {
      print "\n";
    }
  }

  /**
   * Print memory usage if verbose option is specified.
   *
   * @param int $position Position of chunk.
   */
  protected function printMemoryUsage($position) {
    if (!$this->verbose) {
      return;
    }

    $memory = number_format(memory_get_usage() / 1024 / 1024);
    print "Memory usage at position {$position} is {$memory}MB";
    if ($position !== -1) {
      print "\r";
    }
  }

  /**
   * Get a chunk of entities.
   *
   * @return array Chunk of entities. Returns empty array
   */
  public function getEntitiesChunk() {
    $bundle_table = "chado_bio_data_{$this->bundle->bundle_id}";

    $query = db_select($bundle_table, 'CB');
    $query->fields('CB', ['entity_id', 'record_id']);
    $query->orderBy('entity_id', 'asc');
    $query->range($this->offset, $this->chunk);
    $data = $query->execute()->fetchAll();

    return $data;
  }

  /**
   * Eager load all the data associated with the given set of entities.
   *
   * @param array $entities Must contain entity_id and record_id
   *                        from chado_bundle_N tables
   *
   * @return array
   */
  public function loadData($entities) {
    if (empty($entities)) {
      return [];
    }

    $bundle = $this->bundle;

    // Get the record ids as an array
    $record_ids = array_map(function ($entity) {
      return (int) $entity->record_id;
    }, $entities);

    // Get data
    $cvterms = $this->loadCVTerms($bundle->data_table, $record_ids);
    $this->loadCVTermPathParents($cvterms);
    $properties = $this->loadProperties($bundle->data_table, $record_ids);
    $this->loadCVTermPathParents($properties);
    $relatedCvterms = $this->loadRelatedCVTerms($bundle->data_table, $record_ids);
    $this->loadCVTermPathParents($relatedCvterms);
    $relatedProps = $this->loadRelatedProperties($bundle->data_table, $record_ids);
    $this->loadCVTermPathParents($relatedProps);

    // Index by record id
    $data = [];
    foreach ($entities as $entity) {
      $data[$entity->record_id] = [
        'entity_id' => $entity->entity_id,
        'cvterms' => $cvterms[$entity->record_id] ?: [],
        'properties' => $properties[$entity->record_id] ?: [],
        'related_cvterms' => $relatedCvterms[$entity->record_id] ?: [],
        'related_props' => $relatedProps[$entity->record_id] ?: [],
      ];
    }
    return $data;
  }

  /**
   * Chunk the terms by entity and load all related cvterms.
   *
   * @param $terms
   */
  public function loadCVTermPathParents(&$terms) {
    foreach ($terms as $record_id => $records) {
      $this->addCVTermPathRelatedTerms($terms[$record_id]);
    }
  }

  /**
   * Get all related term for the given cvterms.
   *
   * @param $terms
   */
  public function addCVTermPathRelatedTerms(&$terms) {
    $ids = array_map(function ($term) {
      return $term->cvterm_id;
    }, $terms);

    if (empty($ids)) {
      return;
    }

    $query = db_select('chado.cvtermpath', 'CVTP');
    $query->fields('CVT', ['cvterm_id']);
    $query->fields('DB', ['name']);
    $query->fields('DBX', ['accession']);
    $query->join('chado.cvterm', 'CVT', 'CVTP.object_id = CVT.cvterm_id');
    $query->join("chado.dbxref", "DBX", "CVT.dbxref_id = DBX.dbxref_id");
    $query->join("chado.db", "DB", "DBX.db_id = DB.db_id");
    $query->condition('CVTP.subject_id', $ids, 'IN');
    $query->condition('DB.name', 'null', '!=');
    $query->addExpression('subject_id');
    $query->isNotNull('DB.name');
    $query->where('subject_id != object_id');
    $terms += $query->execute()->fetchAll();
  }

  /**
   * Load cvterms for a given set of records in a chado table.
   *
   * @param string $table
   * @param array $record_ids Array of integers referring to
   *                            record_id in chado_bio_data_N
   *
   * @return array cvterm object indexed by record id
   */
  public function loadCVTerms($table, $record_ids) {
    $cvterm_table = "chado.{$table}_cvterm";
    $primary_key = $this->primaryKey($table);

    $query = db_select($cvterm_table, 'CT');
    $query->addField('CT', $primary_key, 'record_id');
    $query->fields('CVT', ['cvterm_id']);
    $query->fields('DB', ['name']);
    $query->fields('DBX', ['accession']);
    $query->join('chado.cvterm', 'CVT', 'CT.cvterm_id = CVT.cvterm_id');
    $query->join("chado.dbxref", "DBX", "CVT.dbxref_id = DBX.dbxref_id");
    $query->join("chado.db", "DB", "DBX.db_id = DB.db_id");
    $query->condition($primary_key, $record_ids, 'IN');
 //   $query->condition('DB.name', 'null', '!=');
    $query->condition('DB.name', $this->cv_shortnames, 'IN');

    $query->isNotNull('DB.name');
    $cvterms = $query->execute()->fetchAll();

    $data = [];
    foreach ($cvterms as $cvterm) {
      $data[$cvterm->record_id][] = $cvterm;
    }

    return $data;
  }

  /**
   * Get properties for a given set of records.
   *
   * @param string $table chado table name
   * @param array $record_ids Record ids
   *
   * @return array
   */
  public function loadProperties($table, $record_ids) {
    $props_table = "chado.{$table}prop";
    $primary_key = $this->primaryKey($table);

    $query = db_select($props_table, 'CT');
    $query->addField('CT', $primary_key, 'record_id');
    $query->fields('CVT', ['cvterm_id']);
    $query->fields('DB', ['name']);
    $query->fields('DBX', ['accession']);
    $query->join('chado.cvterm', 'CVT', 'CT.type_id = CVT.cvterm_id');
    $query->join("chado.dbxref", "DBX", "CVT.dbxref_id = DBX.dbxref_id");
    $query->join("chado.db", "DB", "DBX.db_id = DB.db_id");
    $query->condition($primary_key, $record_ids, 'IN');
  //  $query->condition('DB.name', 'null', '!=');
   // $query->isNotNull('DB.name');
    $query->condition('DB.name', $this->cv_shortnames, 'IN');

    $properties = $query->execute()->fetchAll();

    $data = [];
    foreach ($properties as $property) {
      $data[$property->record_id][] = $property;
    }

    return $data;
  }

  /**
   * Loads all related cvterms from _relationship table.
   *
   * @param $table
   * @param $record_ids
   *
   * @return array
   */
  public function loadRelatedCVTerms($table, $record_ids) {
    $cvterms_by_subject = $this->loadRelatedCvtermsBy('subject_id', $table, $record_ids);
    $cvterms_by_object = $this->loadRelatedCvtermsBy('object_id', $table, $record_ids);
    $added = [];
    $data = [];

    foreach ($cvterms_by_object as $cvterm) {
      // avoid inserting duplicate cvterm ids
      if (!isset($added[$cvterm->object_id][$cvterm->cvterm_id])) {
        $added[$cvterm->object_id][$cvterm->cvterm_id] = TRUE;
        $data[$cvterm->object_id][] = $cvterm;
      }
    }

    foreach ($cvterms_by_subject as $cvterm) {
      // avoid inserting duplicate cvterm ids
      if (!isset($added[$cvterm->subject_id][$cvterm->cvterm_id])) {
        $added[$cvterm->subject_id][$cvterm->cvterm_id] = TRUE;
        $data[$cvterm->subject_id][] = $cvterm;
      }
    }

    return $data;
  }

  /**
   * Loads all related cvterms by subject or object.
   *
   * @param $column
   * @param $table
   * @param $record_ids
   *
   * @return mixed
   */
  public function loadRelatedCvtermsBy($column, $table, $record_ids) {
    $cvterm_table = "chado.{$table}_cvterm";
    $relationship_table = "chado.{$table}_relationship";
    $primary_key = $this->primaryKey($table);
    $opposite_column = $column === 'object_id' ? 'subject_id' : 'object_id';

    $query = db_select($relationship_table, 'RT');
    $query->addField('CT', $primary_key, 'record_id');
    $query->fields('CVT', ['cvterm_id']);
    $query->fields('DB', ['name']);
    $query->fields('DBX', ['accession']);
    $query->fields('RT', [$column]);
    $query->join($cvterm_table, 'CT', 'RT.' . $opposite_column . ' = CT.' . $primary_key);
    $query->join('chado.cvterm', 'CVT', 'CT.cvterm_id = CVT.cvterm_id');
    $query->join("chado.dbxref", "DBX", "CVT.dbxref_id = DBX.dbxref_id");
    $query->join("chado.db", "DB", "DBX.db_id = DB.db_id");
    $query->condition('RT.' . $column, $record_ids, 'IN');
  //  $query->condition('DB.name', 'null', '!=');
  //  $query->isNotNull('DB.name');
    $query->condition('DB.name', $this->cv_shortnames, 'IN');


    return $query->execute()->fetchAll();
  }

  /**
   * Get all related properties from _relationship tables.
   *
   * @param $table
   * @param $record_ids
   *
   * @return array
   */
  public function loadRelatedProperties($table, $record_ids) {
    $properties_by_subject = $this->loadRelatedPropertiesBy('subject_id', $table, $record_ids);
    $properties_by_object = $this->loadRelatedPropertiesBy('object_id', $table, $record_ids);
    $added = [];
    $data = [];

    foreach ($properties_by_object as $property) {
      // avoid inserting duplicate cvterm ids
      if (!isset($added[$property->object_id][$property->cvterm_id])) {
        $added[$property->object_id][$property->cvterm_id] = TRUE;
        $data[$property->object_id][] = $property;
      }
    }

    foreach ($properties_by_subject as $property) {
      // avoid inserting duplicate cvterm ids
      if (!isset($added[$property->subject_id][$property->cvterm_id])) {
        $added[$property->subject_id][$property->cvterm_id] = TRUE;
        $data[$property->subject_id][] = $property;
      }
    }

    return $data;
  }

  /**
   * Get related properties by specifying subject or object
   *
   * @param $column
   * @param $table
   * @param $record_ids
   *
   * @return mixed
   */
  public function loadRelatedPropertiesBy($column, $table, $record_ids) {
    $prop_table = "chado.{$table}prop";
    $relationship_table = "chado.{$table}_relationship";
    $primary_key = $this->primaryKey($table);
    $opposite_column = $column === 'object_id' ? 'subject_id' : 'object_id';

    $query = db_select($relationship_table, 'RT');
    $query->addField('PT', $primary_key, 'record_id');
    $query->fields('CVT', ['cvterm_id']);
    $query->fields('DB', ['name']);
    $query->fields('DBX', ['accession']);
    $query->fields('RT', [$column]);
    $query->join($prop_table, 'PT', 'RT.' . $opposite_column . ' = PT.' . $primary_key);
    $query->join('chado.cvterm', 'CVT', 'PT.type_id = CVT.cvterm_id');
    $query->join("chado.dbxref", "DBX", "CVT.dbxref_id = DBX.dbxref_id");
    $query->join("chado.db", "DB", "DBX.db_id = DB.db_id");
    $query->condition('RT.' . $column, $record_ids, 'IN');
   // $query->condition('DB.name', 'null', '!=');
   // $query->isNotNull('DB.name');
    $query->condition('DB.name', $this->cv_shortnames, 'IN');


    return $query->execute()->fetchAll();
  }

  /**
   * Insert data into the index table.
   *
   * @param array $data Array structured as returned in loadData
   *
   * @see \CVBrowserIndexer::loadData()
   * @throws \Exception
   * @return \DatabaseStatementInterface|int
   */
  public function insertData(&$data) {
    $query = db_insert('tripal_cvterm_entity_linker')->fields([
      'entity_id',
      'cvterm_id',
      'database',
      'accession',
    ]);

    foreach ($data as $record_id => &$record) {
      $entity_id = &$record['entity_id'];
      $cvterms = &$record['cvterms'];
      $properties = &$record['properties'];
      $related_props = &$record['related_props'];
      $related_cvterms = &$record['related_cvterms'];

      foreach ($cvterms as $cvterm) {
        $query->values($this->extractCvtermForInsertion($cvterm, $entity_id));
      }

      foreach ($related_cvterms as $cvterm) {
        $query->values($this->extractCvtermForInsertion($cvterm, $entity_id));
      }

      foreach ($properties as $property) {
        $query->values($this->extractCvtermForInsertion($property, $entity_id));
      }

      foreach ($related_props as $property) {
        $query->values($this->extractCvtermForInsertion($property, $entity_id));
      }
    }

    return $query->execute();
  }

  /**
   * Extracts the needed data for insertion into the linker table.
   *
   * @param $data
   * @param $entity_id
   *
   * @return array
   */
  public function extractCvtermForInsertion($data, $entity_id) {
    return [
      'entity_id' => $entity_id,
      'cvterm_id' => $data->cvterm_id,
      'database' => $data->name,
      'accession' => $data->accession,
    ];
  }

  /**
   * Get and cache primary key of a chado table.
   *
   * @param string $table table name such as "feature".
   *
   * @return mixed|string
   */
  public function primaryKey($table) {
    if (isset(static::$primaryKeys[$table])) {
      return static::$primaryKeys[$table];
    }

    $schema = chado_get_schema($table);
    if (isset($schema['primary key']) && !empty($schema['primary key'])) {
      $key = $schema['primary key'][0];
      static::$primaryKeys[$table] = $key;
      return $key;
    }

    $key = "{$table}_id";
    static::$primaryKeys[$table] = $key;
    return $key;
  }

  /**
   * Set the chunk size.
   *
   * @param int $size Number of elements per chunk.
   */
  public function setChunkSize($size) {
    $this->chunk = $size;
  }

  /**
   * Write a line to STDOUT if verbose mode is enabled.
   *
   * @param $line
   */
  public function write($line) {
    if ($this->verbose) {
      print "$line\n";
    }
  }
}
