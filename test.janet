(import build/tahani :as t)  (def d (t/open "testdb"))

(loop [i :range [0 100000]]
 (t/put d (string "yummy" i) (string "baba ghamoush" i)))

(print "---------------------------")

(loop [i :range [0 100000]]
 (t/get d (string "yummy" i)))

(print "Janet: " (t/get d "yummy999"))

(print "Janet: " (t/get d "nil"))

(t/close d)
