#!/bin/bash
ls -ltr ee_mumu/lib/build.none_*_inl0_hrd* gg_tt/lib/build.none_*_inl0_hrd* gg_tt*g/lib/build.none_*_inl0_hrd* | egrep -v '(total|\./|\.build|_common|^$)'

