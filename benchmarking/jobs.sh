for version in "parallel" "master-worker" "master-worker-async"
do
	oarsub -l nodes=4 "./run.sh $version"
done
