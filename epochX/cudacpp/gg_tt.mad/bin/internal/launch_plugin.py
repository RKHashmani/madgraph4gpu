# Copyright (C) 2020-2023 CERN and UCLouvain.
# Licensed under the GNU Lesser General Public License (version 3 or later).
# Created by: O. Mattelaer (Aug 2023) for the MG5aMC CUDACPP plugin.
# Further modified by: O. Mattelaer, A. Valassi (2023) for the MG5aMC CUDACPP plugin.

import logging
import os
import subprocess
pjoin = os.path.join
logger = logging.getLogger('cmdprint') # for stdout

try:
    import madgraph
except ImportError:
    import internal.madevent_interface as madevent_interface
    import internal.misc as misc
    import internal.extended_cmd as extended_cmd
    import internal.banner as banner_mod
    import internal.common_run_interface as common_run_interface
else:
    import madgraph.interface.madevent_interface as madevent_interface
    import madgraph.various.misc as misc
    import madgraph.interface.extended_cmd as extended_cmd
    import madgraph.various.banner as banner_mod
    import madgraph.interface.common_run_interface as common_run_interface

class CPPMEInterface(madevent_interface.MadEventCmdShell):
    def compile(self, *args, **opts):
        """ """
        import multiprocessing
        if not self.options['nb_core'] or self.options['nb_core'] == 'None':
            self.options['nb_core'] = multiprocessing.cpu_count()    
        if 'cwd' in opts and os.path.basename(opts['cwd']) == 'Source':
            path = pjoin(opts['cwd'], 'make_opts')
            avx_type = self.run_card['avx_type'] if self.run_card['avx_type'] != 'auto' else ''
            common_run_interface.CommonRunCmd.update_make_opts_full(path,
                {'FPTYPE': self.run_card['floating_type'],
                 'AVX':  avx_type })
            misc.sprint('FPTYPE checked')
        if args and args[0][0] == 'madevent' and hasattr(self, 'run_card'):            
            cudacpp_backend = self.run_card['cudacpp_backend'].upper() # the default value is defined in banner.py
            logger.info("Building madevent in madevent_interface.py with '%s' matrix elements"%cudacpp_backend)
            if cudacpp_backend == 'FORTRAN':
                args[0][0] = 'madevent_fortran_link'
            elif cudacpp_backend == 'CPP':
                args[0][0] = 'madevent_cpp_link'
            elif cudacpp_backend == 'CUDA':
                args[0][0] = 'madevent_cuda_link'
            else:
                raise Exception("Invalid cudacpp_backend='%s': only 'FORTRAN', 'CPP', 'CUDA' are supported")
            return misc.compile(nb_core=self.options['nb_core'], *args, **opts)
        else:
            return misc.compile(nb_core=self.options['nb_core'], *args, **opts)

# Phase-Space Optimization ------------------------------------------------------------------------------------
template_on = \
"""#*********************************************************************
# SIMD/GPU Parametrization
#*********************************************************************
   %(floating_type)s = floating_type ! single precision(f), double precision (d), mixed (m) [double for amplitude, single for color]
   %(avx_type)s =  avx_type  ! for SIMD, technology to use for the vectorization
   %(cudacpp_backend)s = cudacpp_backend ! Fortran/CPP/CUDA switch mode to use
"""

template_off = ''
plugin_block = banner_mod.RunBlock('simd', template_on=template_on, template_off=template_off)

class CPPRunCard(banner_mod.RunCardLO):
    blocks = banner_mod.RunCardLO.blocks + [plugin_block]

    def reset_simd(self, old_value, new_value, name):
        if not hasattr(self, 'path'):
            raise Exception('INTERNAL ERROR! CPPRunCard instance has no attribute path') # now ok after fixing #790
        if name == "vector_size" and new_value <= int(old_value):
            # code can handle the new size -> do not recompile
            return
        Sourcedir = pjoin(os.path.dirname(os.path.dirname(self.path)), 'Source')
        subprocess.call(['make', 'cleanavx'], cwd=Sourcedir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def reset_makeopts(self, old_value, new_value, name):
        if not hasattr(self, 'path'):
            raise Exception
        avx_value = self['avx_type'] if self['avx_type'] != 'auto' else ''
        if name == 'floating_type':
            common_run_interface.CommonRunCmd.update_make_opts_full({'FPTYPE': new_value, 'AVX': avx_value})
        elif name == 'avx_type':
            if new_value == 'Auto':
                new_value = ''
            common_run_interface.CommonRunCmd.update_make_opts_full({'FPTYPE': self['floating_type'], 'AVX': new_value})
        else:
            raise Exception
        Sourcedir = pjoin(os.path.dirname(os.path.dirname(self.path)), 'Source')
        subprocess.call(['make', 'cleanavx'], cwd=Sourcedir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def plugin_input(self, finput):
        return

    def default_setup(self):
        super().default_setup()
        self.add_param('floating_type', 'd', include=False, hidden=False,
                       fct_mod=(self.reset_makeopts,(),{}),
                       allowed=['m','d','f'])
        self.add_param('avx_type', 'auto', include=False, hidden=False,
                       fct_mod=(self.reset_makeopts,(),{}),
                       allowed=['auto', 'none', 'sse4', 'avx2','512y','512z'])
        self.add_param('cudacpp_backend', 'CPP', include=False, hidden=False,
                       allowed=['Fortan', 'CPP', 'CUDA'])
        self['vector_size'] = 16 # already setup in default class (just change value)
        self['aloha_flag'] = '--fast-math'
        self['matrix_flag'] = '-O3'
        self.display_block.append('simd')
        self.display_block.append('psoptim')

    # OM/AV - overload the default version in banner.py
    def write_one_include_file(self, output_dir, incname, output_file=None):
        """write one include file at the time"""
        if incname == "vector.inc":
            if 'vector_size' not in self.user_set: return
            if output_file is None: vectorinc=pjoin(output_dir,incname)
            else: vectorinc=output_file
            with open(vectorinc+'.new','w') as fileout:
                with open(vectorinc) as filein:
                    for line in filein:
                        if line.startswith('C'): fileout.write(line)
            super().write_one_include_file(output_dir, incname, output_file)
            with open(vectorinc+'.new','a') as fileout:
                with open(vectorinc) as filein:
                    for line in filein:
                        if not line.startswith('\n'): fileout.write(line)
            os.replace(vectorinc+'.new',vectorinc)
        else:
            super().write_one_include_file(output_dir, incname, output_file)

    def check_validity(self):
        """ensure that PLUGIN information are consistent"""
        super().check_validity()
        if self['SDE_strategy'] != 1:
            logger.warning('SDE_strategy different of 1 is not supported with SMD/GPU mode')
            self['sde_strategy'] = 1
        if self['hel_recycling']:
            self['hel_recycling'] = False

class GPURunCard(CPPRunCard):
    def default_setup(self):
        super().default_setup()
        # change default value:
        self['cudacpp_backend'] = 'CUDA'
        self['vector_size'] = 16384 # already setup in default class (just change value)

MEINTERFACE = CPPMEInterface
RunCard = CPPRunCard
