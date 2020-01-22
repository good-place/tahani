(import ../../build/tahani :as t)
(import jhydro :as j)

(defn hash2hex [data ctx] (freeze (j/util/bin2hex (j/hash/hash 16 data ctx))))

(defn- _make-index [self field data]
  (string field "-" (hash2hex data (self :ctx))))

(defn- _get [self id]
  (with [d (t/open (self :name)) t/close] (-?> (t/record/get d id) (unmarshal))))

(defn- _write [self batch]
  (with [d (t/open (self :name)) t/close] (t/batch/write batch d)))

(defn- save [self data]
  (let [md (freeze (marshal data))
        id (hash2hex md (self :ctx))
        batch (t/batch/create)]
    (t/batch/put batch id md)
    (loop [f :in (self :to-index)
           :let [mf (:_make-index self f (get data f))
                 index (array/push (or (:_get self mf) @[]) id)]]
     (t/batch/put batch mf (freeze (marshal index))))
    (:_write self batch)
    id))

(defn- load [self id] (:_get self id))

(defn- find-by [self field term]
  (seq [id :in (:_get self (:_make-index self field term))] (:_get self id)))

(defn- find-all [self query]
  (seq [[k v] :pairs query] (:find-by self k v)))

(def Store
  @{:name nil :to-index nil :ctx "-tahani-"
    :_make-index _make-index :_get _get :_write _write
    :save save :load load
    :find-by find-by :find-all find-all})

(defn create [name &opt to-index]
  (default to-index [])
  (assert (and (tuple? to-index) (all |(keyword? $) to-index)))
  (-> @{} (table/setproto Store) (merge-into {:name name :to-index to-index})))
