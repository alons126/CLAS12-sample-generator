#!/bin/bash                                                                                                          
#SBATCH --nodes=1                                                                                                    
#SBATCH --ntasks=1                                                                                                   
#SBATCH --mem-per-cpu=2000                                                                                            
#SBATCH --account=clas12                                                                                             
#SBATCH --partition=production                                                               
#SBATCH --time=20:00:00                                                                                               
#SBATCH --output=/farm_out/%u/%x-%j-%N.out                                                                           
#SBATCH --error=//farm_out/%u/%x-%j-%N.err                                                                           

echo "GEMC_DATA_DIR = ${GEMC_DATA_DIR}"
echo
echo "TEMP_BEAM_E = ${TEMP_BEAM_E}"
echo
echo "TEMP_OUTPATH_PARTICLE = ${TEMP_OUTPATH_PARTICLE}"
echo
#Change file prefix for your simulation                                                                                                                          
FILE_PREFIX=Uniform_${TEMP_OUTPATH_PARTICLE}_sample_${TEMP_BEAM_E}
echo "FILE_PREFIX = ${FILE_PREFIX}"
echo

NEVENTS=10000
#-1.0 for inbending(6,4 GeV) 0.5 for outbending (2 Gev)
TORUS=${TORUS_FIELD}
echo "TORUS = ${TORUS}"

#set output file path location, don't forget to set up dir using setupdir.sh
JOB_OUT_PATH=${OUTPATH}
echo "JOB_OUT_PATH = ${JOB_OUT_PATH}"
echo

#choose the Gcard for your target type
GCARD=${GCARD_FILE}
echo "GCARD = ${GCARD}"
#Reconstruction yaml file
YAML=${YAML_FILE}
echo "YAML = ${YAML}"
echo

#------DONT NEED TO TOUCH UNDER HERE UNLESS YOU NEED TOO------
LUNDOUT=${JOB_OUT_PATH}/lundfiles
MCOUT=${JOB_OUT_PATH}/mchipo
RECONOUT=${JOB_OUT_PATH}/reconhipo

#SUBMIT GEMC MC
gemc -USE_GUI=0  -SCALE_FIELD="binary_torus, $TORUS" -SCALE_FIELD="binary_solenoid, -1.0" -N=$NEVENTS -INPUT_GEN_FILE="lund, ${LUNDOUT}/${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}.txt" -OUTPUT="hipo, ${MCOUT}/mc_${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}_torus$TORUS.hipo" $GCARD

#RECONSTRUCTION
recon-util -y $YAML -n $NEVENTS -i ${MCOUT}/mc_${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}_torus${TORUS}.hipo -o ${RECONOUT}/recon_${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}_torus${TORUS}.hipo
