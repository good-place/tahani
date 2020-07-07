(declare-project
  :name "tahani"
  :author "Josef Pospíšil"
  :repo "git+https://github.com/good-place/tahani.git"
  :description "LevelDB wrapper for Janet"
  :dependencies ["https://github.com/janet-lang/jhydro.git"])

(declare-native
  :name "tahani"
  :lflags ["-lleveldb"]
  :source @["tahani.c"])
