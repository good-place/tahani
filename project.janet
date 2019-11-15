(declare-project
  :name "tahani"
  :description "LevelDB wrapper for Janet")

(declare-native 
 :name "tahani" 
 :cflags ["-Ileveldb/db"]
 :lflags ["-lleveldb"]
 :source @["tahani.c"])
