(import build/tahani :as t)  

(def db-name "testdb" )

(def d (t/open db-name))

(def reqs 1_000_000)

(loop [i :range [0 reqs]]
 (t/put d (string "yummy" i) (string "baba ghamoush" i)))

(loop [i :range [(- reqs (/ reqs 10)) reqs]]
 (t/delete d (string "yummy" i)))

(print "899th yummy is: " (t/get d "yummy899"))

(print "99999th yummy is eaten: " (t/get d "yummy999999"))

(t/close d)

(t/destroy db-name)
