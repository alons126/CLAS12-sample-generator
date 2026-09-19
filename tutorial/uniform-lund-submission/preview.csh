set OUT = /lustre24/expphy/volatile/clas12/asportes/Analysis_output/Uniform_samples

foreach label (1e enFD epFD)
    source run.csh --workflow submit --build false \
      --manifest "$OUT/Uniform_sample_${label}_2070MeV/lundfiles/lund-gen-monitoring/lund-gen-log.json" \
      --gcard config/detector/Generation_files_2GeV/5.14/rgm_fall2021_Ar_2GeV.gcard \
      --reconstruction config/detector/Generation_files_2GeV/5.14/rgm_fall2021-cv.yaml \
      --site config/sites/jlab.json \
      --torus 0.5 --solenoid -1 --execute
end

foreach label (1e enFD epFD)
    source run.csh --workflow submit --build false \
      --manifest "$OUT/Uniform_sample_${label}_4029MeV/lundfiles/lund-gen-monitoring/lund-gen-log.json" \
      --gcard config/detector/Generation_files_4GeV/5.14/rgm_fall2021_Ar_4GeV.gcard \
      --reconstruction config/detector/Generation_files_4GeV/5.14/rgm_fall2021-ai_4Gev.yaml \
      --site config/sites/jlab.json \
      --torus -1 --solenoid -1 --execute
end

foreach label (1e enFD epFD)
    source run.csh --workflow submit --build false \
      --manifest "$OUT/Uniform_sample_${label}_5986MeV/lundfiles/lund-gen-monitoring/lund-gen-log.json" \
      --gcard config/detector/Generation_files_6GeV/5.14/rgm_fall2021_Ar_6GeV.gcard \
      --reconstruction config/detector/Generation_files_6GeV/5.14/rgm_fall2021-ai_6Gev.yaml \
      --site config/sites/jlab.json \
      --torus -1 --solenoid -1 --execute
end