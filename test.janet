(import build/tahani :as t)  (def d (t/open "testdb"))

(t/put d "yummy" "baba ghamoush")

(print "---------------------------")

(print "Janet: " (t/get d "yummy"))

(print "Janet: " (t/get d "nil"))

(t/close d)
