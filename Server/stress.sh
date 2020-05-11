#!/bin/bash
for i in {1..10000}
do
   ./net -c "Requestig #t $i"
done
