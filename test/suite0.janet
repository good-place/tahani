(import test/helper :prefix "" :exit true)

(import ../build/tahani :as t)

(start-suite 0)

(def db-name "testdb")

(defer (t/manage/destroy db-name)
  (def d (t/open db-name))
  (assert d "DB is not opened")
  (t/record/put d "yummy" "baba ghamoush")
  (assert (= (t/record/get d "yummy") "baba ghamoush") "Record is not saved")
  (t/record/delete d "yummy")
  (assert (nil? (t/record/get d "yummy")) "Record is not deleted")
  (assert (= (string d) "name=testdb state=opened") "Database state is not opened")
  (:close d)
  (assert (= (string d) "name=testdb state=closed") "Database state is not closed"))

(defer (t/manage/destroy db-name)
  (with [d (t/open db-name :eie)]
    (assert d "DB is not opened with option error if exist")
    (:close d)
    (assert-error "Does not panic with error if exist option" (t/open db-name :eie))))

(defer (t/manage/destroy db-name)
  (with [d (t/open db-name)]
    (def b (t/batch/create))
    (assert b "Batch is not created")
    (t/record/put d "HOHOHO" "Santa")
    (-> b
      (:put "HEAT" "Summer")
      (:delete "HOHOHO")
      (:write d)
      (:destroy))
    (assert (nil? (t/record/get d "HOHOHO")) "Record is not deleted by batch")
    (assert (t/record/get d "HEAT") "Record is not inserted by batch")))

(defer (t/manage/destroy db-name)
  (with [_ (t/open db-name)] )
  (assert (first (protect (t/manage/repair db-name))) "DB is not repaired"))

(defer (t/manage/destroy db-name)
  (with [d (t/open db-name)]
        (def s (t/snapshot/create d))
        (assert s "Snapshot is not created")
        (assert-no-error "Snapshot is not released" (t/snapshot/release s))
        (assert-no-error (:release (t/snapshot/create d)))))

(end-suite)
