(declare-project
  :name "tahani"
  :description "LevelDB wrapper for Janet"
  :dependencies ["https://github.com/janet-lang/jhydro.git"])

(declare-native
 :name "tahani"
 :cflags ["-Ileveldb/db"]
 :lflags ["-lleveldb"]
 :source @["tahani.c"])

(declare-source
 :name "tahani"
 :source @["tahani/"])

# run repl with tahani included
(phony "repl" ["build"] (os/execute ["janet" "-r" "-e" "(import build/tahani :as t)"] :p))
