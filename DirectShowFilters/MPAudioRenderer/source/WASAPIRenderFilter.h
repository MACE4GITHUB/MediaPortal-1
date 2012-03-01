// Copyright (C) 2005-2010 Team MediaPortal
// http://www.team-mediaportal.com
// 
// MediaPortal is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// MediaPortal is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with MediaPortal. If not, see <http://www.gnu.org/licenses/>.

#pragma once 

#include "stdafx.h"
#include "IRenderFilter.h"
#include "Settings.h"
#include "queuedaudiosink.h"
#include "SyncClock.h"

#define MPAR_S_NEED_DATA        ((HRESULT)0x00040201)
#define MPAR_S_DROP_SAMPLE      ((HRESULT)0x00040202)
#define MPAR_S_RENDER_SAMPLE    ((HRESULT)0x00040203)
#define MPAR_S_WAIT_RENDER_TIME ((HRESULT)0x00040204)
#define CLOCK_DATA_SIZE 10
#define MAX_SAMPLE_WAIT_TIME 10 // 10 ms

using namespace std;

class CWASAPIRenderFilter : public CQueuedAudioSink, public IRenderFilter
{
public:
  CWASAPIRenderFilter(AudioRendererSettings* pSettings, CSyncClock* pClock);
  virtual ~CWASAPIRenderFilter();

  // IAudioSink implementation
  HRESULT Init();
  HRESULT Cleanup();
  HRESULT NegotiateFormat(const WAVEFORMATEXTENSIBLE* pwfx, int nApplyChangesDepth);
  HRESULT EndOfStream();

  // IRenderFilter implementation
  HRESULT AudioClock(ULONGLONG& pTimestamp, ULONGLONG& pQpc);
  REFERENCE_TIME Latency();

protected:
  // Processing
  DWORD ThreadProc();

// Internal implementation
private:

  enum RenderState
  {
    StateStopped,
    StateRunning,
    StatePaused
  };

  // AVRT.dll (Vista or greater)
  typedef HANDLE (__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
  typedef BOOL (__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

  HRESULT EnableMMCSS();
  HRESULT RevertMMCSS();

  PTR_AvSetMmThreadCharacteristicsW	pfAvSetMmThreadCharacteristicsW;
  PTR_AvRevertMmThreadCharacteristics	pfAvRevertMmThreadCharacteristics;

  HMODULE m_hLibAVRT;

  HRESULT GetAudioDevice(IMMDevice** ppMMDevice);
  HRESULT GetAvailableAudioDevices(IMMDeviceCollection** ppMMDevices, bool pLog); // caller must release ppMMDevices!
  HRESULT CreateAudioClient(IMMDevice* pMMDevice, IAudioClient** ppAudioClient);
  HRESULT InitAudioClient(const WAVEFORMATEX* pWaveFormatEx, IAudioRenderClient** ppRenderClient);
  HRESULT StartAudioClient(IAudioClient** ppAudioClient);
  HRESULT StopAudioClient(IAudioClient** ppAudioClient);
  void CancelDataEvent();
  void ReleaseResources();

  void ResetClockData();
  void UpdateAudioClock();

  HRESULT CheckAudioClient(WAVEFORMATEX* pWaveFormatEx);
  HRESULT CheckSample(IMediaSample* pSample, UINT32 framesToFlush);
  HRESULT CheckStreamTimeline(IMediaSample* pSample, REFERENCE_TIME* pDueTime, UINT32 sampleOffset);
  HRESULT GetBufferSize(const WAVEFORMATEX* pWaveFormatEx, REFERENCE_TIME* pHnsBufferPeriod);
  void CalculateSilence(REFERENCE_TIME* pDueTime, LONGLONG* pBytesOfSilence);
  void RenderAudio(BYTE* pTarget, UINT32 bufferSizeInBytes, UINT32 &dataLeftInSample, UINT32 &sampleOffset, IMediaSample* pSample, UINT32 &bytesFilled);
  void RenderSilence(BYTE* pTarget, UINT32 bufferSizeInBytes, LONGLONG &writeSilence, UINT32 &bytesFilled);

  AudioRendererSettings* m_pSettings;
  IMMDevice*          m_pMMDevice;
  IAudioClient*       m_pAudioClient;
  IAudioRenderClient* m_pRenderClient;
  UINT32              m_nFramesInBuffer;
  HANDLE              m_hTask;
  UINT32              m_nBufferSize;
  IAudioClock*        m_pAudioClock;
  UINT64              m_nHWfreq;
  DWORD               m_dwStreamFlags;

  bool                m_bIsAudioClientStarted;

  HANDLE              m_hDataEvent;
  HANDLE              m_hTimerEvent;

  DWORD_PTR           m_dwAdvise;

  RenderState         m_state;

  CSyncClock*         m_pClock;

  REFERENCE_TIME      m_rtNextSampleTime;

  // Audio HW clock data
  CCritSec            m_csClockLock;
  UINT64              m_ullHwClock[CLOCK_DATA_SIZE];
  UINT64              m_ullHwQpc[CLOCK_DATA_SIZE];
  UINT64              m_dClockDataCollectionCount;
  int                 m_dClockPosIn;
  int                 m_dClockPosOut;

  vector<HANDLE> m_hDataEvents;
  vector<HANDLE> m_hSampleEvents;

  vector<DWORD> m_dwDataWaitObjects;
  vector<DWORD> m_dwSampleWaitObjects;
};
