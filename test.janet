(import build/tahani :as t)  

(def db-name "testdb" )

# DB operations
(def d (t/open db-name))
(def reqs 100_000)
(print " ===== Saving " reqs " yummies")
(loop [i :range [0 reqs]]
 (t/record/put d (string "yummy" i) (string "baba ghamoush" i)))
(loop [i :range [(- reqs (/ reqs 10)) reqs]]
 (t/record/delete d (string "yummy" i)))
(print "899th yummy is: " (t/record/get d "yummy899"))
(print "99999th yummy is eaten: " (t/record/get d "yummy999999"))

# Batch operations
(print " ===== Batching")
(def b (t/batch/create))
(t/batch/put b "HOHOHO" "Santa")
(t/batch/delete b "yummy899")
(t/batch/write d b)
(t/batch/destroy b)
(print "Xmass " (t/record/get d "HOHOHO"))
(print "899th yummy is eaten: " (t/record/get d "yummy899"))

# Management operations
(t/close d)
(t/manage/destroy db-name)
