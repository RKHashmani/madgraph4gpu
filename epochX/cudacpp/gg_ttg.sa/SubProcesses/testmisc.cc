// Copyright (C) 2020-2023 CERN and UCLouvain.
// Licensed under the GNU Lesser General Public License (version 3 or later).
// Created by: A. Valassi (Jan 2022) for the MG5aMC CUDACPP plugin.
// Further modified by: J. Teig, A. Valassi (2022-2023) for the MG5aMC CUDACPP plugin.
//----------------------------------------------------------------------------
// Use ./runTest.exe --gtest_filter=*misc to run only testmisc.cc tests
//----------------------------------------------------------------------------

#include "mgOnGpuConfig.h"

#include "mgOnGpuVectors.h"

#include "epoch_process_id.h"

#include <gtest/gtest.h>

#include <sstream>
#include <typeinfo>

#ifdef MGONGPUCPP_GPUIMPL
#define TESTID( s ) s##_GPU_MISC
#else
#define TESTID( s ) s##_CPU_MISC
#endif

#define XTESTID( s ) TESTID( s )

// NB: namespaces mg5amcGpu and mg5amcCpu includes types which are defined in different ways for CPU and GPU builds (see #318 and #725)
#ifdef MGONGPUCPP_GPUIMPL
namespace mg5amcGpu
#else
namespace mg5amcCpu
#endif
{
#ifdef MGONGPU_CPPSIMD /* clang-format off */
#define EXPECT_TRUE_sv( cond ) { bool_v mask( cond ); EXPECT_TRUE( maskand( mask ) ); }
#else
#define EXPECT_TRUE_sv( cond ) { EXPECT_TRUE( cond ); }
#endif /* clang-format on */

  inline const std::string
  boolTF( const bool& b )
  {
    return ( b ? "T" : "F" );
  }

#ifdef MGONGPU_CPPSIMD
  inline const std::string
  boolTF( const bool_v& v )
  {
    std::stringstream out;
    out << "{ " << ( v[0] ? "T" : "F" );
    for( int i = 1; i < neppV; i++ ) out << ", " << ( v[i] ? "T" : "F" );
    out << " }";
    return out.str();
  }
#endif
}

TEST( XTESTID( MG_EPOCH_PROCESS_ID ), testmisc )
{
#ifdef MGONGPUCPP_GPUIMPL
  using namespace mg5amcGpu;
#else
  using namespace mg5amcCpu;
#endif

  //--------------------------------------------------------------------------

  EXPECT_TRUE( true );

  //--------------------------------------------------------------------------

  // Vector initialization for fptype_sv
  {
    fptype_sv f{ 0 };
    EXPECT_TRUE_sv( f == 0 );
  }
  {
    fptype_sv f = fptype_sv{ 0 };
    EXPECT_TRUE_sv( f == 0 );
  }

  // Vector initialization for fptype_sv - demonstrate bug #339 in older cxmake implementation
  {
    fptype_sv f{ 1 };
    //std::cout << f << std::endl << boolTF( f == 1 ) << std::endl;
    //EXPECT_TRUE_sv( f == 1 ); // this fails for vectors! TFFF
#ifndef MGONGPU_CPPSIMD
    EXPECT_TRUE_sv( f == 1 ); // this succeds: T
#else
    EXPECT_TRUE( ( f == 1 )[0] ); // this succeds: TFFF[0]
    EXPECT_TRUE( ( f[0] == 1 ) );
    for( int i = 1; i < neppV; i++ )
    {
      EXPECT_TRUE( !( ( f == 1 )[i] ) ); // this succeds: FTTT[i>=1]
      EXPECT_TRUE( ( f[i] == 0 ) );      // equals 0, not 1
    }
#endif
  }

#ifdef MGONGPU_CPPSIMD
  // Vector initialization for cxtype_sv - demonstrate fix for bug #339
  {
    fptype_sv f1 = fptype_v{ 0 } + 1;
    EXPECT_TRUE_sv( f1 == 1 );
    cxtype_v c12 = cxmake( f1, 2 );
    //std::cout << c12 << std::endl << boolTF( c12.real() == 1 ) << std::endl << boolTF( c12.imag() == 2 ) << std::endl;
    EXPECT_TRUE_sv( c12.real() == 1 );
    EXPECT_TRUE_sv( c12.imag() == 2 );
    cxtype_v c21 = cxmake( 2, f1 );
    //std::cout << c21 << std::endl << boolTF( c21.real() == 2 ) << std::endl << boolTF( c21.imag() == 1 ) << std::endl;
    EXPECT_TRUE_sv( c21.real() == 2 );
    EXPECT_TRUE_sv( c21.imag() == 1 );
  }
#endif

  // Vector initialization for cxtype_sv
  {
    cxtype_sv c = cxzero_sv();
    EXPECT_TRUE_sv( c.real() == 0 );
    EXPECT_TRUE_sv( c.imag() == 0 );
  }
  {
    cxtype_sv c = cxmake( 1, fptype_sv{ 0 } ); // here was a bug #339
    EXPECT_TRUE_sv( c.real() == 1 );
    EXPECT_TRUE_sv( c.imag() == 0 );
  }
  {
    cxtype_sv c = cxmake( fptype_sv{ 0 }, 1 ); // here was a bug #339
    EXPECT_TRUE_sv( c.real() == 0 );
    EXPECT_TRUE_sv( c.imag() == 1 );
  }

  // Array initialization for cxtype_sv array (example: jamp_sv in CPPProcess.cc)
  {
    cxtype_sv array[2] = {}; // all zeros (NB: vector cxtype_v IS initialized to 0, but scalar cxype is NOT, if "= {}" is missing!)
    //std::cout << array[0].real() << std::endl; std::cout << boolTF( array[0].real() == 0 ) << std::endl;
    EXPECT_TRUE_sv( array[0].real() == 0 );
    EXPECT_TRUE_sv( array[0].imag() == 0 );
    EXPECT_TRUE_sv( array[1].real() == 0 );
    EXPECT_TRUE_sv( array[1].imag() == 0 );
  }

  // Alternative array initialization for cxtype_sv array (example: was used for outwf in testxxx.cc)
  {
    cxtype_sv array[2]{}; // all zeros (NB: vector cxtype_v IS initialized to 0, but scalar cxype is NOT, if "{}" is missing!)
    //std::cout << array[0].real() << std::endl; std::cout << boolTF( array[0].real() == 0 ) << std::endl;
    EXPECT_TRUE_sv( array[0].real() == 0 );
    EXPECT_TRUE_sv( array[0].imag() == 0 );
    EXPECT_TRUE_sv( array[1].real() == 0 );
    EXPECT_TRUE_sv( array[1].imag() == 0 );
  }

  //--------------------------------------------------------------------------

  // Scalar complex references
  {
    using namespace mgOnGpu;
    // Refs to f1, f2
    fptype f1 = 1;
    fptype f2 = 2;
    cxtype_ref r12( f1, f2 ); // copy refs
    //cxtype_ref r12a( r12 ); //deleted
    cxtype_ref r12a( cxtype_ref( f1, f2 ) ); // copy refs
    //cxtype_ref r12b = r12; // deleted
    cxtype_ref r12b = cxtype_ref( f1, f2 ); // copy refs
    EXPECT_TRUE( cxtype( r12 ).real() == 1 );
    EXPECT_TRUE( cxtype( r12 ).imag() == 2 );
    EXPECT_TRUE( cxtype( r12a ).real() == 1 );
    EXPECT_TRUE( cxtype( r12a ).imag() == 2 );
    EXPECT_TRUE( cxtype( r12b ).real() == 1 );
    EXPECT_TRUE( cxtype( r12b ).imag() == 2 );
    // Refs to f1c, f2c
    fptype f1c = 0;
    fptype f2c = 0;
    cxtype_ref r12c( f1c, f2c );
    EXPECT_TRUE( cxtype( r12c ).real() == 0 );
    EXPECT_TRUE( cxtype( r12c ).imag() == 0 );
    //r12c = r12; // deleted
    r12c = cxtype( r12 ); // copy values
    EXPECT_TRUE( cxtype( r12c ).real() == 1 );
    EXPECT_TRUE( cxtype( r12c ).imag() == 2 );
    // Update f1, f2
    f1 = 10;
    f2 = 20;
    EXPECT_TRUE( cxtype( r12 ).real() == 10 );
    EXPECT_TRUE( cxtype( r12 ).imag() == 20 );
    EXPECT_TRUE( cxtype( r12a ).real() == 10 );
    EXPECT_TRUE( cxtype( r12a ).imag() == 20 );
    EXPECT_TRUE( cxtype( r12b ).real() == 10 );
    EXPECT_TRUE( cxtype( r12b ).imag() == 20 );
    EXPECT_TRUE( cxtype( r12c ).real() == 1 ); // points to f1c, not to f1
    EXPECT_TRUE( cxtype( r12c ).imag() == 2 ); // points to f2c, not to f2
  }

  // Vector complex references
  {
    using namespace mgOnGpu;
    // Refs to f1, f2
    fptype_sv f1 = fptype_sv{ 0 } + 1;
    fptype_sv f2 = fptype_sv{ 0 } + 2;
    cxtype_sv_ref r12( f1, f2 ); // copy refs
    //cxtype_sv_ref r12a( r12 ); //deleted
    cxtype_sv_ref r12a( cxtype_sv_ref( f1, f2 ) ); // copy refs
    //cxtype_sv_ref r12b = r12; // deleted
    cxtype_sv_ref r12b = cxtype_sv_ref( f1, f2 ); // copy refs
    EXPECT_TRUE_sv( cxtype_sv( r12 ).real() == 1 );
    EXPECT_TRUE_sv( cxtype_sv( r12 ).imag() == 2 );
    EXPECT_TRUE_sv( cxtype_sv( r12a ).real() == 1 );
    EXPECT_TRUE_sv( cxtype_sv( r12a ).imag() == 2 );
    EXPECT_TRUE_sv( cxtype_sv( r12b ).real() == 1 );
    EXPECT_TRUE_sv( cxtype_sv( r12b ).imag() == 2 );
    // Refs to f1c, f2c
    fptype_sv f1c = fptype_sv{ 0 };
    fptype_sv f2c = fptype_sv{ 0 };
    cxtype_sv_ref r12c( f1c, f2c );
    EXPECT_TRUE_sv( cxtype_sv( r12c ).real() == 0 );
    EXPECT_TRUE_sv( cxtype_sv( r12c ).imag() == 0 );
    //r12c = r12; // deleted
    r12c = cxtype_sv( r12 ); // copy values
    EXPECT_TRUE_sv( cxtype_sv( r12c ).real() == 1 );
    EXPECT_TRUE_sv( cxtype_sv( r12c ).imag() == 2 );
    // Update f1, f2
    f1 = fptype_sv{ 0 } + 10;
    f2 = fptype_sv{ 0 } + 20;
    EXPECT_TRUE_sv( cxtype_sv( r12 ).real() == 10 );
    EXPECT_TRUE_sv( cxtype_sv( r12 ).imag() == 20 );
    EXPECT_TRUE_sv( cxtype_sv( r12a ).real() == 10 );
    EXPECT_TRUE_sv( cxtype_sv( r12a ).imag() == 20 );
    EXPECT_TRUE_sv( cxtype_sv( r12b ).real() == 10 );
    EXPECT_TRUE_sv( cxtype_sv( r12b ).imag() == 20 );
    EXPECT_TRUE_sv( cxtype_sv( r12c ).real() == 1 ); // points to f1c, not to f1
    EXPECT_TRUE_sv( cxtype_sv( r12c ).imag() == 2 ); // points to f2c, not to f2
  }

  //--------------------------------------------------------------------------

  // Boolean vector (mask) times FP vector
  /*
  // From https://github.com/madgraph5/madgraph4gpu/issues/765#issuecomment-1853672838
  channelids_sv = CHANNEL_ACCESS::kernelAccess( pchannelIds ); // the 4 channels in the SIMD vector
  bool_sv mask_sv = ( channelids_sv == 1 );
  numerators_sv += mask_sv * cxabs2( amp_sv[0] );
  if( pchannelIds != nullptr ) denominators_sv += cxabs2( amp_sv[0] );
  */
  {
    typedef bool_sv test_int_sv;  // defined as scalar_or_vector of long int (FPTYPE=double) or int (FPTYPE=float)
    test_int_sv channelids0_sv{}; // mimic CHANNEL_ACCESS::kernelAccess( pchannelIds )
    test_int_sv channelids1_sv{}; // mimic CHANNEL_ACCESS::kernelAccess( pchannelIds )
    fptype_sv absamp0_sv{};       // mimic cxabs2( amp_sv[0] )
    fptype_sv absamp1_sv{};       // mimic cxabs2( amp_sv[0] )
#ifdef MGONGPU_CPPSIMD
    for( int i = 0; i < neppV; i++ )
    {
      channelids0_sv[i] = i;   // 0123
      channelids1_sv[i] = i;   // 1234
      absamp0_sv[i] = 10. + i; // 10. 11. 12. 13.
      absamp1_sv[i] = 11. + i; // 11. 12. 13. 14.
    }
#else
    channelids0_sv = 0;
    channelids1_sv = 1;
    absamp0_sv = 10.;
    absamp1_sv = 11.;
#endif
    bool_sv mask0_sv = ( channelids0_sv % 2 == 0 ); // even channels 0123 -> TFTF (1010)
    bool_sv mask1_sv = ( channelids1_sv % 2 == 0 ); // even channels 1234 -> FTFT (0101)
    constexpr fptype_sv fpZERO_sv{};                // 0000
    //fptype_sv numerators0_sv = mask0_sv * absamp0_sv; // invalid operands to binary * ('__vector(4) long int' and '__vector(4) double')
    fptype_sv numerators0_sv = fpternary( mask0_sv, absamp0_sv, fpZERO_sv ); // equivalent to "mask0_sv * absamp0_sv"
    fptype_sv numerators1_sv = fpternary( mask1_sv, absamp1_sv, fpZERO_sv ); // equivalent to "mask1_sv * absamp1_sv"
#ifdef MGONGPU_CPPSIMD
    //std::cout << "numerators0_sv: " << numerators0_sv << std::endl;
    //std::cout << "numerators1_sv: " << numerators1_sv << std::endl;
    for( int i = 0; i < neppV; i++ )
    {
      // Values of numerators0_sv: 10.*1 11.*0 12.*1 13.*0
      if( channelids0_sv[i] % 2 == 0 ) // even channels
        EXPECT_TRUE( numerators0_sv[i] == ( 10. + i ) );
      else // odd channels
        EXPECT_TRUE( numerators0_sv[i] == 0. );
      // Values of numerators1_sv: 11.*0 12.*1 13.*0 14.*1
      if( channelids1_sv[i] % 2 == 0 ) // even channels
        EXPECT_TRUE( numerators1_sv[i] == ( 11. + i ) );
      else // odd channels
        EXPECT_TRUE( numerators1_sv[i] == 0. );
    }
#else
    // Values of numerators0_sv: 10.*1
    EXPECT_TRUE( numerators0_sv == 10. );
    // Values of numerators1_sv: 11.*0
    EXPECT_TRUE( numerators1_sv == 0. );
#endif
  }

  //--------------------------------------------------------------------------
}
