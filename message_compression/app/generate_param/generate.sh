ERR_SEED=`date +%N`
#python generate_gauss_param.py ${ERR_SEED} 1024 ./1K/param 256
ERR_SEED=`date +%N`
#python generate_gauss_param.py ${ERR_SEED} 10240 ./10K/param 256
ERR_SEED=`date +%N`
#python generate_gauss_param.py ${ERR_SEED} 102400 ./100K/param 256
ERR_SEED=`date +%N`
python generate_gauss_param.py ${ERR_SEED} 1048576 ./1M/param 256
ERR_SEED=`date +%N`
#python generate_gauss_param.py ${ERR_SEED} 10485760 ./10M/param 256
#ERR_SEED=`date +%N`
#python generate_gauss_param.py ${ERR_SEED} 104857600 ./100M/param 256
