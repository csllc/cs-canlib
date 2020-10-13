/*
 *                   Copyright 2002-2019 by KVASER AB, SWEDEN
 *
 *                        WWW: http://www.kvaser.com
 *
 * This software is furnished under a license and may be used and copied
 * only in accordance with the terms of such license.
 *
 */

/**
 *
 * \file kvDiag.h
 * \brief Definitions for the Diagnostics API.
 * \note The kvDiag API is preliminary and will change!
 *
 */

#ifndef _KVDIAG_H_
#define _KVDIAG_H_

#include <canlib.h>
#include <stdint.h>

/**
 * \defgroup grp_kvdiag  kvdiag
 * \brief Diagnostics API Library.
 * \defgroup kvdiag_setup    Setup
 * \brief Allocate buffers, attach analyzers.
 * \ingroup grp_kvdiag
 * \defgroup kvdiag_diagnostics    Diagnostics
 * \brief Start, stop and download triggers.
 * \ingroup grp_kvdiag
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \name DIAG_ANALYZER_TYPE_xxx
 * Analyzer type.
 * \anchor DIAG_ANALYZER_TYPE_xxx
 * @{
 */
#define DIAG_ANALYZER_TYPE_DEFAULT 0 /**< Default. */
/** @} */

/**
 * \name KDI_MAX_SAMPLE_SIZE
 * The maximum size of sample from a default analyzer.
 * Used with \ref kvDiagReadSampleRaw.
 * @{
 */
#define KDI_DEFAULT_MAX_SAMPLE_SIZE 8196
/** @} */

/**
 * \name DiagDataPtr
 *
 * Pointer to diagnostic data buffer.
 * @{
 */
typedef void* DiagDataPtr;
/** @} */

/// @cond 
/// @private
typedef struct {
  double bitrate;
  uint32_t quality;
} bitrate_t;

/// @private
typedef struct {
  bitrate_t btr1;
  bitrate_t btr2;
} bitrates_t;

/// @private
typedef struct {
  struct {
    uint32_t version;
    uint32_t seqno;
    uint32_t type;
  } header;
  struct {
    uint32_t id;
    uint64_t time;
    uint8_t data[64];
    uint32_t flag;
    uint8_t dlc;
    uint8_t padding[3];
  } msg;
  struct {
    uint64_t startTime;
    uint32_t sampleFreq;
    uint8_t  startValue;
    uint8_t  padding;
    uint16_t edgeCount;
    uint32_t edgeTimes[8192];
  } sample;
} kvDiagSample;
/// @endcond 

/**
 * \ingroup kvdiag_setup
 * Attach an analyzer to a CAN channel and prevent other channels from using it.
 *
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd  An open handle to a CAN channel.
 * \param[in] type  \ref DIAG_ANALYZER_TYPE_xxx
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagDetach()
*/
canStatus CANLIBAPI kvDiagAttach(const CanHandle hnd, uint32_t type);

/**
 * \ingroup kvdiag_setup
 * Detach an analyzer from a channel.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd  An open handle to a CAN channel.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagAttach()
 */
canStatus CANLIBAPI kvDiagDetach(const CanHandle hnd);

/**
 * \ingroup kvdiag_setup
 * Allocate diagnostic data buffer. The buffer can be connected to an
 * analyzer with \ref kvDiagConfigure()
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[out] diagData  Pointer to a \ref DiagDataPtr.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagDealloc()
 */
canStatus CANLIBAPI kvDiagAlloc(DiagDataPtr *diagData);

/**
 * \ingroup kvdiag_setup
 * Deallocate diagnostic data buffer.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] diagData  Pointer to a \ref DiagDataPtr.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagAlloc()
 */
canStatus CANLIBAPI kvDiagDealloc(DiagDataPtr *diagData);

/**
 * \ingroup kvdiag_diagnostics
 * Download a trigger to an attached analyzer and connect it to a
 * diagnostic data buffer. There are two types of triggers:
 *
 * 1. Autobaud trigger. This trigger is used to calculate bitrate.
 *
 * \code{.c}
 *    const char *TRIGGER_AUTOBAUD = "trigger=autobaud\n";
 * \endcode
 *
 * 2. Normal trigger. This trigger is used when the bitrate is known, e.g.:
 * \code{.c}
 *    const char *TRIGGER_1MBIT = "trigger=normal\n"
 *                                "tseg1=59\n"
 *                                "tseg2=20\n"
 *                                "sjw=16\n"
 *                                "brp=1\n"
 *                                "nbsample=-1\n";
 * \endcode
 *    where
 * \code{.unparsed}
 *     tseg1     Time segment 1, that is, the number of quanta from (but not
 *               including) the Sync Segment to the sampling point.
 *
 *     tseg2     Time segment 2, that is, the number of quanta from the sampling
 *               point to the end of the bit.
 *
 *     sjw       The Synchronization Jump Width.
 *
 *     brp       Bitrate prescaler.
 *
 *     nbsample  Number of samples before trigger stops or use -1 to never stop.
 * \endcode
 *
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd           An open handle to a CAN channel with an attached analyzer.
 * \param[in] configBuffer  A null terminated buffer containing a trigger
 *                          configuration, see TRIGGER_AUTOBAUD and TRIGGER_1MBIT.
 * \param[in] diagData      A diagnostic data buffer allocated with \ref kvDiagAlloc().

 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagAlloc(), \ref kvDiagAttach()
 */
canStatus CANLIBAPI kvDiagConfigure(const CanHandle hnd,
                                    const char *configBuffer,
                                    DiagDataPtr diagData);

/**
 * \ingroup kvdiag_diagnostics
 * Start data acquisition.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd An open handle to a CAN channel with an attached analyzer.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagConfigure() \ref kvDiagStop()
 */
canStatus CANLIBAPI kvDiagStart(const CanHandle hnd);

/**
 * \ingroup kvdiag_diagnostics
 * Stop data acquisition.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd An open handle to a CAN channel with an attached analyzer.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagStart()
 */
canStatus CANLIBAPI kvDiagStop(const CanHandle hnd);


/// @cond 
/// @private
canStatus CANLIBAPI kvDiagReadSample (DiagDataPtr diagData,
                                      kvDiagSample *sample);
/// @endcond 

/**
 * \ingroup kvdiag_diagnostics
 *
 * Read (and consume) one full sample from an analyzer. The sample is written
 * to a buffer, and sampleLength is updated with the length of the sample.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] diagData      Pointer to a \ref DiagDataPtr.
 * \param[out] buffer        Pointer to buffer to receive the sample.
 * \param[in] bufferLength  The length of the buffer.
 * \param[out] sampleLength The length of read sample.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagAlloc(), \ref kvDiagConfigure(), \ref kvDiagStart()
 */
canStatus CANLIBAPI kvDiagReadSampleRaw (DiagDataPtr diagData, uint8_t *buffer,
                                         uint32_t bufferLength,
                                         int *sampleLength);
/// @cond 
/// @private
canStatus CANLIBAPI kvDiagCalculateBitrate(DiagDataPtr diagData, bitrates_t *bitrate);
/// @endcond 

/// @cond 
/// @private
canStatus CANLIBAPI kvDiagResetBitrateCalculation(DiagDataPtr diagData);
/// @endcond 

/// @cond 
/// @private
canStatus CANLIBAPI kvDiagCalculateClockOffset(DiagDataPtr diagData,
                                                  uint32_t id,
                                                  double *fMin,
                                                  double *fMax,
                                                  double *fMean,
                                                  double *fStd,
                                                  uint32_t *n);
/// @endcond 

/// @cond 
/// @private
canStatus CANLIBAPI kvDiagResetClockToleranceCalculation(void *diagData);
/// @endcond 

/**
 * \ingroup kvdiag_diagnostics
 * Get the number of analyzers.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd An open handle to a CAN channel.
 * \param[out] analyzerCount Number of analyzers.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagGetAnalyzerInfo()
 */
canStatus CANLIBAPI kvDiagGetNumberOfAnalyzers(const CanHandle hnd,
                                               uint8_t *analyzerCount);

/**
 * \ingroup kvdiag_diagnostics
 * Get the number of analyzers.
 *
 * \note The kvDiag API is preliminary and will change!
 *
 * \param[in] hnd An open handle to a CAN channel.
 * \param[in] analyzerNo The numbering is zero based.
 * \param[out] type The analyzer type.
 * \param[out] version The analyzer version.
 *
 * \return \ref canOK (zero) if success
 * \return \ref canERR_xxx (negative) if failure
 *
 * \sa \ref kvDiagGetNumberOfAnalyzers()
 */
canStatus CANLIBAPI kvDiagGetAnalyzerInfo (const CanHandle hnd,
                                             uint8_t analyzerNo,
                                             uint32_t *type,
                                             uint32_t *version);

#ifdef __cplusplus
}
#endif

#endif // _KVDIAG_H_
