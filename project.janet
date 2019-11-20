(declare-project
  :name "tahani"
  :description "LevelDB wrapper for Janet")

(declare-native 
 :name "tahani" 
 :cflags ["-Ileveldb/db"]
 :lflags ["-lleveldb"]
 :source @["tahani.c"])

# run repl with tahani included
(phony "repl" ["build"] (os/execute ["janet" "-r" "-e" "(import build/tahani :as t)"] :p))
