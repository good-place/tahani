(import ../../build/tahani :as t)
(import jhydro :as j)

(def ctx "-tahani-")

(defn- make-index [field data]
  (string field "-" (j/util/bin2hex (j/hash/hash 16 (marshal (string data)) ctx))))

(defn- save [self data]
  (def md (string (marshal data)))
  (def id (string (j/util/bin2hex (j/hash/hash 16 md ctx))))
  (def batch (t/batch/create))
  (t/batch/put batch id md)

  (when (self :to-index)
    (loop [f :in (self :to-index)]
      (let [mf (make-index f (get data f))
            old-index (t/record/get (self :db) mf)
            new-index (if old-index (unmarshal old-index) @[])]
        (array/push new-index id)
        (t/batch/put batch mf (string (marshal new-index))))))
  (t/batch/write batch (self :db))
  id)

(defn- search [self field term]
  (seq [id :in (unmarshal (t/record/get (self :db) (make-index field term)))]
    (unmarshal (t/record/get (self :db) id))))

(defn- load [self id]
  (unmarshal (t/record/get (self :db) id)))

(defn- close [self]
  (t/close (self :db)))

(def Store
  @{:name nil
    :db nil
    :to-index nil
    :state nil
    :save save
    :load load
    :search search
    :close close})

(defn create [name &opt to-index]
  (when to-index (assert (tuple? to-index)))
  (-> @{}
      (table/setproto Store)
      (put :name name)
      (put :db (t/open name))
      (put :to-index to-index)))
