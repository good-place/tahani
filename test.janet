(import build/tahani :as t)
(import tahani/store :as ts)

(def db-name "testdb" )

# DB operations
(def d (t/open db-name))
(def reqs 100_000)
(print "\n ===== Saving " reqs " yummies")
(loop [i :range [0 reqs]]
  (t/record/put d (string "yummy" i) (string "baba ghamoush" i)))
(loop [i :range [(- reqs (/ reqs 10)) reqs]]
  (t/record/delete d (string "yummy" i)))
(print "899th yummy is: " (t/record/get d "yummy899"))
(print "99999th yummy is eaten: " (t/record/get d "yummy999999"))
(print "DB status: " (string d))

# Batch operations
(print " ===== Batching")
(-> (t/batch/create)
    (t/batch/put "HOHOHO" "Santa")
    (t/batch/delete "yummy899")
    (t/batch/write d)
    (t/batch/destroy))

(print "We have Xmass " (t/record/get d "HOHOHO"))
(print "899th yummy is eaten by him: " (t/record/get d "yummy899"))

# Big batch
(print " ====== Big Batching ")
(def b (t/batch/create))
(loop [i :range [0 reqs]]
  (t/batch/put b (string "big" i) (string "Oh so big batch" i))
  (when (> 10_000 i) (t/batch/delete b (string "yummy" i))))
(t/batch/write b d)

(print "We have some big batches: " (t/record/get d "big99999"))
# Management operations
(t/close d)
(print "DB status: " (string d))
(t/manage/destroy db-name)

# Store
(print " ====== Store ")
(def s (ts/create "peopletest" [:name :job]))
(def pid (:save s {:name "Pepe" :job "Programmer"}))
(prin "pepe from the store: " pid " - ")
(pp (:load s pid))
(:save s {:name "Jose" :job "Programmer"})
(:save s {:name "Karl" :job "Gardener"})
(print " === Search by name")
(pp (:search s :name "Pepe"))
(print " === Search by job")
(pp (:search s :job "Programmer"))
(:close s)
(t/manage/destroy "peopletest")
