#/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc allreduce_lm.c -o allreduce_lm_simgrid

rm allreduce_lm_thres_redbcast_simgrid
rm allreduce_lm_simgrid
rm allreduce_lm_thres_rscateragather1_simgrid
rm allreduce_lm_thres_rscateragather11_simgrid

/home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpicc  allreduce_lm_thres_rscateragather1.c -o  allreduce_lm_thres_rscateragather1_simgrid
/home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpicc  allreduce_lm_thres_rscateragather11.c -o  allreduce_lm_thres_rscateragather11_simgrid
/home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpicc allreduce_lm_thres_redbcast.c -o allreduce_lm_thres_redbcast_simgrid
/home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpicc allreduce_lm.c -o allreduce_lm_simgrid