(import test/helper :prefix "" :exit true)

(import ../build/tahani :as t)

(start-suite 0)

(def db-name "testdb")

# Basic db operations
(defer (t/manage/destroy db-name)
  (def d (t/open db-name))
  (assert d "DB is not opened")
  (t/record/put d "yummy" "baba ghamoush")
  (assert (= (t/record/get d "yummy") "baba ghamoush") "Record is not saved")
  (t/record/delete d "yummy")
  (assert (nil? (t/record/get d "yummy")) "Record is not deleted")
  (assert (= (string d) "name=testdb state=opened") "Database state is not opened")
  (:close d)
  (assert-error "Can put to closed DB" (t/record/put d "yummy" "baba ghamoush"))
  (assert-error "Can get from closed DB" (t/record/get d "yummy"))
  (assert-error "Can delete to closed DB" (t/record/delete d "yummy"))
  (assert (= (string d) "name=testdb state=closed") "Database state is not closed"))

# Open with error_if_exist
(defer (t/manage/destroy db-name)
  (with [d (t/open db-name :eie)]
    (assert d "DB is not opened with option error if exist")
    (:close d)
    (assert-error "Does not panic with error if exist option" (t/open db-name :eie))))

# Batch operations
(defer (t/manage/destroy db-name)
  (with [d (t/open db-name)]
    (def b (t/batch/create))
    (assert b "Batch is not created")
    (t/record/put d "HOHOHO" "Santa")
    (-> b
      (:put "HEAT" "Summer")
      (:delete "HOHOHO")
      (:write d)
      )
    (assert (nil? (t/record/get d "HOHOHO")) "Record is not deleted by batch")
    (assert (t/record/get d "HEAT") "Record is not inserted by batch")
    (:destroy b)
    (assert-error "Can write destroyed batch" (:write b d))
    (:close d)
    (assert-error "Can write batch to closed db" (:write b d))))

# Repair DB
(defer (t/manage/destroy db-name)
  (with [_ (t/open db-name)] )
  (assert (first (protect (t/manage/repair db-name))) "DB is not repaired"))

# Snapshot operations
(defer (t/manage/destroy db-name)
  (with [d (t/open db-name)]
    (def s (t/snapshot/create d))
    (assert s "Snapshot is not created")
    (assert-no-error "Snapshot is not released" (t/snapshot/release s))
    (assert-no-error "Snapshot is not released" (:release s))
    (:close d)
    (assert-error "Can create snapshot from closed db" (t/snapshot/create d))))

# Iterator operations
(defer (t/manage/destroy db-name)
  (with [d (t/open db-name)]
    (t/record/put d "HOHOHO" "Santa")
    (t/record/put d "HEAT" "Summer")
    (def i (t/iterator/create d))
    (assert i "Iterator is not created")
    (assert (not (t/iterator/valid? i)) "Iterator is valid before seek")
    (assert-no-error "Iterator does not seek to first" (t/iterator/seek-to-first i))
    (assert (t/iterator/valid? i) "Iterator is not valid after seek")
    (assert (= (t/iterator/key i) "HEAT") "First key is not HEAT")
    (assert (= (t/iterator/value i) "Summer") "First value is not Summer")
    (assert-no-error "Iterator does not seek to last" (t/iterator/seek-to-last i))
    (assert (= (t/iterator/key i) "HOHOHO") "Last key is not HOHOHO")
    (assert (= (t/iterator/value i) "Santa") "Last value is not Santa")
    (assert-no-error "Iterator does not seek next" (t/iterator/next i))
    (assert (not (t/iterator/valid? i)) "Iterator is valid after last")
    (t/iterator/seek-to-first i)
    (t/iterator/next i)
    (assert (= (t/iterator/key i) "HOHOHO") "Last key is not HOHOHO")
    (assert-no-error "Iterator does not seek prev" (t/iterator/prev i))
    (assert (= (t/iterator/key i) "HEAT") "First key is not HEAT")
    (assert-no-error "Iterator does not seek key" (t/iterator/seek i "HOHOHO"))
    (assert (= (t/iterator/value i) "Santa") "Seeked value is not Santa")
    (t/iterator/destroy i)
    (def snapshot (t/snapshot/create d))
    (def si (t/iterator/create d snapshot))
    (assert si "Iterator with snapshot is not created")
    (:put d "MEAT" "All")
    (:seek-to-last si)
    (assert (= (:key si) "HOHOHO") "Snapshot Iterator has wrong last key")
    (:release snapshot)
    (:destroy si)
    (assert-error "Can seek first on destroyed iterator" (:seek-to-first i))
    (assert-error "Can seek last on destroyed iterator" (:seek-to-last i))
    (assert-error "Can next on destroyed iterator" (:next i))
    (assert-error "Can prev on destroyed iterator" (:prev i))
    (assert-error "Can seek on destroyed iterator" (:seek i "HOHOHO"))
    (assert-error "Can get key on destroyed iterator" (:key i))
    (assert-error "Can get value on destroyed iterator" (:value i))
    (:close d)
    (assert-error "Can create iterator from closed db" (t/iterator/create d))))

(end-suite)
