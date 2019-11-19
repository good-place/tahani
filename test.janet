(import build/tahani :as t)  

(def db-name "testdb" )

(def d (t/open db-name))

(print "Creating one milion yummies")
(loop [i :range [0 1000000]]
 (t/put d (string "yummy" i) (string "baba ghamoush" i)))

(print "Eating last hundreth thousands yummies")
(loop [i :range [900000 1000000]]
 (t/delete d (string "yummy" i)))

(print "899th yummy is: " (t/get d "yummy899"))

(print "99999th yummy is eaten: " (t/get d "yummy99999"))

(t/close d)

(t/destroy db-name)
