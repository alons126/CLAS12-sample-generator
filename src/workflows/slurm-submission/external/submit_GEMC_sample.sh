#!/bin/bash                                                                                                          
#SBATCH --nodes=1                                                                                                    
#SBATCH --ntasks=1                                                                                                   
#SBATCH --mem-per-cpu=2000                                                                                            
#SBATCH --account=clas12                                                                                             
#SBATCH --partition=production                                                               
#SBATCH --time=20:00:00                                                                                               
#SBATCH --output=/farm_out/%u/%x-%j-%N.out                                                                           
#SBATCH --error=//farm_out/%u/%x-%j-%N.err                                                                           

JOB_TARGET=${SAMPLE_TARGET_NUCLEUS}
# Align non-path values in column 29; keep path printouts in their existing format.
printf '%-28s%s\n' 'JOB_TARGET:' "$JOB_TARGET"
JOB_GENERATOR=${SAMPLE_GENERATOR}
printf '%-28s%s\n' 'JOB_GENERATOR:' "$JOB_GENERATOR"
JOB_GENERATOR_TUNE=${GENERATOR_TUNE}
printf '%-28s%s\n' 'JOB_GENERATOR_TUNE:' "$JOB_GENERATOR_TUNE"
JOB_Q2_CUT=${Q2_CUT}
printf '%-28s%s\n' 'JOB_Q2_CUT:' "$JOB_Q2_CUT"
printf '%-28s%s\n' 'BEAM_ENERGY_LABEL:' "$BEAM_ENERGY_LABEL"
echo "GEMC_DATA_DIR:          ${GEMC_DATA_DIR}"
printf '%-28s%s\n' 'UNIFORM_SAMPLE_CHANNEL:' "$UNIFORM_SAMPLE_CHANNEL"
echo

#Change file prefix for your simulation
FILE_PREFIX=${SAMPLE_FILE_PREFIX}
printf '%-28s%s\n' 'FILE_PREFIX:' "$FILE_PREFIX"
echo

NEVENTS=${JOB_NEVENTS:?JOB_NEVENTS is required}
printf '%-28s%s\n' 'NEVENTS:' "$NEVENTS"
echo

#-1.0 for inbending (6,4 GeV) 0.5 for outbending (2 Gev)
TORUS=${TORUS_FIELD}
printf '%-28s%s\n' 'TORUS:' "$TORUS"
echo

#set output file path location, don't forget to set up dir using setupdir.sh
JOB_OUT_PATH=${OUTPATH}
echo "JOB_OUT_PATH:           ${JOB_OUT_PATH}"
echo

#choose the Gcard for your target type
GCARD=${GCARD_FILE}
echo "GCARD:                  ${GCARD}"
#Reconstruction yaml file
YAML=${YAML_FILE}
echo "YAML:                   ${YAML}"
echo

# Report the inherited selections and executable paths without requiring the module shell function.
# Requested releases and module names describe the environment, not binary version queries.
printf '%-28s%s\n' 'Requested GEMC version:' "$GEMC_VERSION"
printf '%-28s%s\n' 'Requested COATJAVA version:' "$COATJAVA_VERSION"
printf '%-28s%s\n' 'Inherited module selections:' "$LOADEDMODULES"
printf 'GEMC executable: %s\n' "$(command -v gemc)"
printf 'COATJAVA reconstruction executable: %s\n' "$(command -v recon-util)"
echo

#------DONT NEED TO TOUCH UNDER HERE UNLESS YOU NEED TOO------
LUNDOUT=${JOB_OUT_PATH}/lundfiles
MCOUT=${JOB_OUT_PATH}/mchipo
RECONOUT=${JOB_OUT_PATH}/reconhipo

#SUBMIT GEMC MC
gemc -USE_GUI=0  -SCALE_FIELD="binary_torus, $TORUS" -SCALE_FIELD="binary_solenoid, -1.0" -N=$NEVENTS -INPUT_GEN_FILE="lund, ${LUNDOUT}/${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}.txt" -OUTPUT="hipo, ${MCOUT}/mc_${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}_torus$TORUS.hipo" $GCARD

#RECONSTRUCTION
recon-util -y $YAML -n $NEVENTS -i ${MCOUT}/mc_${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}_torus${TORUS}.hipo -o ${RECONOUT}/recon_${FILE_PREFIX}_${SLURM_ARRAY_TASK_ID}_torus${TORUS}.hipo
