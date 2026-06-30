#!/bin/sh
N=$1
shift 1
awk -v n=$N '
NR==1 {
  count = 0
  in_quote = 0
  val = ""
  for (i = 1; i <= NF; i++) {
    field = $i
    if (in_quote) {
      val = val " " field
      if (field ~ /"$/) {
        in_quote = 0
        count++
        if (count == n) {
          print val
          exit
        }
      }
    } else if (field ~ /^"/ && field !~ /"$/) {
      in_quote = 1
      val = field
    } else {
      count++
      if (count == n) {
        print field
        exit
      }
    }
  }
}
' $@ | tr -d '"'
