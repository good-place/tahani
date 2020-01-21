(import ../../build/tahani :as t)

(defn open [name]
  (t/open name))

(defn close [store]
  (t/close store))

(defn put [store data]
  (def md (string (marshal data)))
  (def id (string (hash md)))
  (t/record/put store id md)
  id)

(defn get [store id]
  (unmarshal (t/record/get store id)))
