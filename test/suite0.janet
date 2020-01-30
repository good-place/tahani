(import test/helper :prefix "" :exit true)

(import ../build/tahani :as t)
(import ../tahani/store :as ts)
(import ../tahani/utils :as tu)

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

#@todo split
(defer (t/manage/destroy "peopletest")
  (def s (ts/create "peopletest" [:name :job :pet]))
  (assert s "Store is not created")
  (def id (:save s {:name "Pepe" :job "Programmer" :pet "Cat"}))
  (assert id "Record is not saved")
  (assert (string? id) "Record id is not string")
  (def r (:load s id))
  (assert r "Record is not loaded")
  (assert (struct? r) "Record is not struct")
  (assert (= (r :name) "Pepe") "Record has bad name")
  # @todo transactions
  (:save s {:name "Jose" :job "Programmer" :pet "Cat"})
  (:save s {:name "Karl" :job "Gardener" :pet "Dog"})
  (:save s {:name "Pepe" :job "Gardener" :pet "Dog"})
  (:save s {:name "Joker" :job "Gardener" :pet "" :good-deeds []})
  (def rs (:find-by s :name "Pepe"))
  (assert (array? rs) "Records are not found by find-by")
  (assert (= (length rs) 2) "Not all records are not found by find-by")
  (each ro rs (assert (= (ro :name) "Pepe") "Record with other name is found by find-by"))
  (def ra (:find-all s {:job "Programmer" :name "Pepe"}))
  (assert (array? ra) "Records are not found")
  (assert (all |(array? $) ra) "Records are not in sets")
  (assert (all (fn [set] (find |(= ($ :name) "Pepe") set)) ra) "Record is not in both sets")
  (def ru (tu/union (:find-all s {:job "Programmer" :name "Pepe"})))
  (assert ru "Sets are not in union")
  (assert (= (length ru) 3) "Not all records are not unioned")
  (def ri (tu/intersect (:find-all s {:job "Programmer" :name "Pepe"})))
  (assert ri "Sets are not in intersection")
  (assert (= (length ri) 1) "Records are not intersected right"))

(end-suite)
