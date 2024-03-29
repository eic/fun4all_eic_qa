# > Combiner.csh
# - Creates a folder EvalFiles containing all the output root files from the condor jobs
# - The failed and incomplete jobs are stored at `brownJobs/` and `brownJobs/redJobs/` for investigation
# - The combined statistics are stored at merged_Eval_<detector>.root
# - Output file - merged_Eval_<detector>.root, folders as described above

# v1.0

# Authors
# Siddhant Rathi   (me190003061@iiti.ac.in)
# Sagar Joshi      (ee190002054@iiti.ac.in)

#!/usr/local/bin/tcsh
 setenv HOME /eic/u/$LOGNAME
 source /etc/csh.login
 foreach i (/etc/profile.d/*.csh)
   source $i
 end
 source $HOME/.login
 source /cvmfs/eic.opensciencegrid.org/default/opt/fun4all/core/bin/eic_setup.csh -n

set FILE = condor.out
set STRING = condorjob done
set nJobs = noOfJobsCombiner
set j = 0

mkdir EvalFiles
mkdir brownJobs
cd brownJobs
mkdir redJobs
cd ../
mv macros macros0

while ($j < $nJobs)
   # echo inloop
    cd macros$j
    grep -q "$STRING" $FILE
    if ( $status != 0 ) then
	 # echo then
	 cd ../
         # echo cd
 	 mv macros$j brownJobs/macros$j
	 echo Moved held macros$j
    else
         # echo found
	 set lineNo = `grep -n "$STRING" $FILE | cut -d : -f 1`
	 if($lineNo <= 1000) then
	    grep -n "$STRING" $FILE
	    cd ../
	    mv macros$j brownJobs/redJobs/macros$j
	    echo Moved erroneous macros$j
	 else
	    grep -n "$STRING" $FILE
	    cd ../
	 endif
    endif
    @ j++
end

echo Deletion done

set j = 0

#folder not created if pre-existing
while ($j < $nJobs)
    mkdir macros$j
    @ j++
end

echo Creation done

set j = 0
set count = 0

while ($j < $nJobs)
   # echo inloop
    cd macros$j
    if ( -f Eval_CEMC.root  ) then
	 cp Eval_CEMC.root ../EvalFiles/Eval_CEMC_$count.root
	 cp Eval_FEMC.root ../EvalFiles/Eval_FEMC_$count.root
	 cp Eval_EEMC.root ../EvalFiles/Eval_EEMC_$count.root
	 cp Eval_FHCAL.root ../EvalFiles/Eval_FHCAL_$count.root
	 cp Eval_HCALIN.root ../EvalFiles/Eval_HCALIN_$count.root
	 cp Eval_HCALOUT.root ../EvalFiles/Eval_HCALOUT_$count.root
	 echo extracted from macros$j
	 @ count++
    else
         echo Empty Directory
    endif
    cd ../
    @ j++
end

sed -i "s/successfulJobs/$count/g" hadd.C
echo $count
echo Done Moving


 root.exe -q -b hadd.C\(\"EEMC\"\)
 root.exe -q -b hadd.C\(\"CEMC\"\)
 root.exe -q -b hadd.C\(\"FEMC\"\)
 root.exe -q -b hadd.C\(\"HCALIN\"\)
 root.exe -q -b hadd.C\(\"HCALOUT\"\)
 root.exe -q -b hadd.C\(\"FHCAL\"\)

echo Statistics Combined


