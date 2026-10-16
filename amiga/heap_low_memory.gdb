# Expected release rejection during preparation, before hardware takeover.
# Exit 1 prevents the launcher from treating this as completed gameplay.
break *pop2CoreStopped
continue
if $pc != (unsigned long)&pop2CoreStopped
 echo HEAP LOW MEMORY WRONG STOP\n
 quit 2
end
printf "HEAP LOW MEMORY reason=%s exception=%lu pc=%08lx\n",g_coreReason,g_coreException,g_coreFaultPC
printf "HEAP LOW MEMORY preparation stage=%lu error=%lu vector=%lu fields=%lu\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_scaffoldFields
echo HEAP LOW MEMORY OBSERVED\n
detach
quit 1
