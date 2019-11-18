(import build/tahani :as t)  
(def db-name "testdb" )
(def d (t/open db-name))

(loop [i :range [0 100000]]
 (t/put d (string "yummy" i) (string "baba ghamoush" i)))

(loop [i :range [90000 100000]]
 (t/delete d (string "yummy" i)))

(print "Get 899th yummy: " (t/get d "yummy899"))

(print "Get 99999th yummy: " (t/get d "yummy99999"))

(t/close d)

(t/destroy db-name)
