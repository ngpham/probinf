for i in ./data/fb.txt
do
  for j in 0.7
  do
    ./infl -f $i -k 8 -a 0 -rs 50 -rsin 1 -rstest 2 -p $j -alg maxprobinfl -numsamp 128 \
    -numsamptest 16
    ./infl -f $i -k 8 -a 0 -rs 50 -rsin 1 -rstest 2 -p $j -alg maxprobbicritinfl -numsamp 128 \
    -numsamptest 16
  done
done
echo "All done!"
