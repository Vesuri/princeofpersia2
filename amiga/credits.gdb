# Item 23: idle second attract pass through the original credits page.
# Release build on a1200-030 with no input; read-only observation at
# byte-guarded original sites matching tools/mac_credits.lua. The FS-UAE stub
# drops memory writes and accepts at most sixteen breakpoints.
# Check with tools/check_credits.py RUN ref/mame/runs/credits-4.
set remotetimeout 3600
set $pages=0
set $pens=0
set $nis=0
set $after=0
printf "OBSERVER READY zone=%lu\n",g_coreZoneBytes
break pop2UIOverlayShown
commands
 silent
 printf "UI OVERLAY id=%lu\n",g_uiOverlay
 continue
end
break pop2CoreStopped
commands
 silent
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx reason=%s exception=%lu pc=%08lx\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreReason,g_coreException,g_coreFaultPC
 printf "CORE STATE level=%u pages=%lu pens=%lu nis=%lu after=%lu\n",*(unsigned short*)(g_coreA5-0x4fd2),$pages,$pens,$nis,$after
 dump binary memory @RUN@/stop-screen.bin g_integrationScreen g_integrationScreen+307200
 set $colors=*(unsigned long*)g_integrationColors
 dump binary memory @RUN@/stop-colors.bin $colors $colors+2056
 dump binary memory @RUN@/stop-a5.bin g_coreA5-g_coreBelow g_coreA5
 detach
 quit 0
end
break pop2ScaffoldRestored
commands
 silent
 printf "NORMAL EXIT\n"
 detach
 quit 0
end
tbreak pop2CoreSegmentLoaded if g_coreLoadedSegment==17
continue
set $c17=g_coreSegments[17][0]
if *(unsigned long*)($c17+0x6e42)!=0x4e560000
 echo CREDITS INVALID SEED SITE\n
 detach
 quit 1
end
if *(unsigned short*)($c17+0x1c4e)!=0x381f || *(unsigned short*)($c17+0x6e1e)!=0x361f
 echo CREDITS INVALID RANDOM SITE\n
 detach
 quit 1
end
break *($c17+0x1c4e)
commands
 silent
 set $step=*(unsigned short*)(g_coreA5-0x349a)
 if $step>=0xff && $step<=0x113
  printf "DEMO RANDOM step=%u site=1c4e result=%04x seed=%08lx caller=%08lx\n",$step,*(unsigned short*)$sp,*(unsigned long*)(g_coreA5-0x18fc),*(unsigned long*)($a6+4)
 end
 continue
end
break *($c17+0x6e1e)
commands
 silent
 set $step=*(unsigned short*)(g_coreA5-0x349a)
 if $step>=0xff && $step<=0x113
  printf "DEMO RANDOM step=%u site=6e1e result=%04x seed=%08lx caller=%08lx\n",$step,*(unsigned short*)$sp,*(unsigned long*)(g_coreA5-0x18fc),*(unsigned long*)($a6+4)
 end
 continue
end
tbreak pop2CoreSegmentLoaded if g_coreLoadedSegment==15
continue
set $c2=g_coreSegments[2][0]
printf "SEG2 base=%08lx\n",$c2
if *(unsigned long*)($c2+0x2b70)!=0x4e56fff0 || *(unsigned short*)($c2+0x2da8)!=0x3d6e || *(unsigned short*)($c2+0x2c20)!=0x4a46 || *(unsigned short*)($c2+0x2d02)!=0x3006 || *(unsigned short*)($c2+0x475c)!=0x544f || *(unsigned short*)($c2+0x476e)!=0x544f || *(unsigned short*)($c2+0x4780)!=0x544f || *(unsigned short*)($c2+0x4576)!=0x3800 || *(unsigned short*)($c2+0x68b2)!=0x4e75
 echo CREDITS INVALID SITE\n
 detach
 quit 1
end
break *($c2+0x2b70)
commands
 silent
 printf "CREDITS ENTER level=%u minutes=%u\n",*(unsigned short*)(g_coreA5-0x4faa),*(unsigned short*)(g_coreA5-0x4fce)
 continue
end
break *($c2+0x2da8)
commands
 silent
 set $pens=$pens+1
 set $pst=$a6-0x14
 printf "CREDITS PENSTATE %lu loc=%04x,%04x size=%04x,%04x mode=%04x pat=%08lx%08lx\n",$pens,*(unsigned short*)$pst,*(unsigned short*)($pst+2),*(unsigned short*)($pst+4),*(unsigned short*)($pst+6),*(unsigned short*)($pst+8),*(unsigned long*)($pst+10),*(unsigned long*)($pst+14)
 continue
end
break *($c2+0x2c20)
commands
 silent
 set $pages=$pages+1
 printf "CREDITS PAGE %lu d5=%u d6=%u\n",$pages,$d5&65535,$d6&65535
 eval "dump binary memory @RUN@/credits-page-%u-screen.bin g_integrationScreen g_integrationScreen+307200",$pages
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/credits-page-%u-colors.bin $colors $colors+2056",$pages
 eval "dump binary memory @RUN@/credits-page-%u-a5.bin g_coreA5-g_coreBelow g_coreA5",$pages
 continue
end
break *($c2+0x2d02)
commands
 silent
 printf "CREDITS RETURN result=%u pages=%lu pens=%lu\n",$d6&65535,$pages,$pens
 continue
end
break *($c2+0x475c)
commands
 silent
 set $nis=$nis+1
 printf "ATTRACT NIS 7 result=%u\n",$d0&65535
 continue
end
break *($c2+0x476e)
commands
 silent
 set $nis=$nis+1
 printf "ATTRACT NIS 4 result=%u\n",$d0&65535
 continue
end
break *($c2+0x4780)
commands
 silent
 set $nis=$nis+1
 printf "ATTRACT NIS 8 result=%u\n",$d0&65535
 continue
end
break *($c2+0x4576)
commands
 silent
 printf "ATTRACT RETURN result=%u level=%u pages=%lu nis=%lu\n",$d0&65535,*(unsigned short*)(g_coreA5-0x4faa),$pages,$nis
 if $pages>0
  set $after=1
 end
 continue
end
break *($c2+0x68b2)
commands
 silent
 if $pages==0
  printf "DEMO FRAME step=%u seed=%08lx level=%u room=%u clock=%u\n",*(unsigned short*)(g_coreA5-0x349a),*(unsigned long*)(g_coreA5-0x18fc),*(unsigned short*)(g_coreA5-0x4fd2),*(unsigned short*)(g_coreA5-0x4fc2),*(unsigned short*)(g_coreA5-0x4fca)
 end
 if $after>0
  set $after=$after+1
  if $after==3
   printf "CREDITS CYCLE COMPLETE level=%u room=%u\n",*(unsigned short*)(g_coreA5-0x4fd2),*(unsigned short*)(g_coreA5-0x4fc2)
   printf "CREDITS SEGMENTS 4=%08lx 5=%08lx 17=%08lx\n",g_coreSegments[4][0],g_coreSegments[5][0],g_coreSegments[17][0]
   dump binary memory @RUN@/credits-after-screen.bin g_integrationScreen g_integrationScreen+307200
   set $colors=*(unsigned long*)g_integrationColors
   dump binary memory @RUN@/credits-after-colors.bin $colors $colors+2056
   dump binary memory @RUN@/credits-after-a5.bin g_coreA5-g_coreBelow g_coreA5
   detach
   quit 0
  end
 end
 continue
end
tbreak pop2CoreSegmentLoaded if g_coreLoadedSegment==4
continue
set $c4=g_coreSegments[4][0]
if *(unsigned short*)($c4+0x3c6a)!=0x4a40 || *(unsigned short*)($c4+0x3c78)!=0x4a40
 echo CREDITS INVALID SOUND SITE\n
 detach
 quit 1
end
break *($c4+0x3c6a)
commands
 silent
 printf "DEMO SOUNDON step=%u site=3c6a result=%04x\n",*(unsigned short*)(g_coreA5-0x349a),$d0&65535
 continue
end
break *($c4+0x3c78)
commands
 silent
 printf "DEMO SOUNDON step=%u site=3c78 result=%04x\n",*(unsigned short*)(g_coreA5-0x349a),$d0&65535
 continue
end
continue
