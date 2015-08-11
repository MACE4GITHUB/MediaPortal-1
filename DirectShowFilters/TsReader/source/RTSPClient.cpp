#include <afx.h>
#include <afxwin.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <streams.h>
#include "tsreader.h"
#include "rtspclient.h"
#include "MemorySink.h"

// For more details for memory leak detection see the alloctracing.h header
#include "..\..\alloctracing.h"


extern void LogDebug(const char *fmt, ...) ;
extern DWORD m_tGTStartTime;


#define TIMEOUT_GENERIC_RTSP_RESPONSE 500   // unit = milliseconds


CRTSPClient::CRTSPClient(CMemoryBuffer& buffer)
  : m_buffer(buffer)
{
  LogDebug("CRTSPClient::CRTSPClient()");
  m_isBufferThreadActive = false;
  m_duration = 7200 * 1000;
  m_session = NULL;
  m_ourClient = NULL;
  m_isPaused = false;

  m_genericResponseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  m_durationDescribeResponseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  TaskScheduler* scheduler = MPTaskScheduler::createNew();
  if (scheduler == NULL) 
  {
    LogDebug("CRTSPClient::CRTSPClient(): failed to create task scheduler");
    return;
  }

  m_env = BasicUsageEnvironment::createNew(*scheduler);
  if (m_env == NULL) 
  {
    LogDebug("CRTSPClient::CRTSPClient(): failed to create usage environment");
    delete scheduler;
    return;
  }
}

CRTSPClient::~CRTSPClient()
{
  LogDebug("CRTSPClient::~CRTSPClient()");
  Shutdown();
  if (m_env)
  {
    TaskScheduler* scheduler = &m_env->taskScheduler();
    m_env->reclaim();
    m_env = NULL;
    delete scheduler;
  }
  if (m_genericResponseEvent != NULL)
  {
    CloseHandle(m_genericResponseEvent);
    m_genericResponseEvent = NULL;
  }
  if (m_durationDescribeResponseEvent != NULL)
  {
    CloseHandle(m_durationDescribeResponseEvent);
    m_durationDescribeResponseEvent = NULL;
  }
}

void SubsessionAfterPlaying(void* clientData) 
{
  LogDebug("CRTSPClient::SubsessionAfterPlaying()");
}

void SubsessionByeHandler(void* clientData) 
{
  LogDebug("CRTSPClient::SubsessionByeHandler()");
}

bool CRTSPClient::SetupStreams()
{
  LogDebug("CRTSPClient::SetupStreams()");
  bool result = false;
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) 
  {
    if (subsession->clientPortNum() == 0) continue; // port # was not set

    LogDebug("CRTSPClient::SetupStreams(): send RTSP SETUP");
    ResetEvent(m_genericResponseEvent);
    m_ourClient->sendSetupCommand(*subsession, &CRTSPClient::OnGenericResponseReceived);
    if (WaitForSingleObject(m_genericResponseEvent, TIMEOUT_GENERIC_RTSP_RESPONSE) == WAIT_TIMEOUT)
    {
      LogDebug("CRTSPClient::SetupStreams(): RTSP SETUP timed out");
    }
    else if (m_genericResponseResultCode != 0)
	  {
      LogDebug("CRTSPClient::SetupStreams(): RTSP SETUP failed, result code = %d, message = %s", m_genericResponseResultCode, m_env->getResultMsg());
    }
    else
    {
      LogDebug("CRTSPClient::SetupStreams(): RTSP SETUP succeeded, port = %d", subsession->clientPortNum());
      result = true;
    }
  }
  return result;
}

void CRTSPClient::Shutdown()
{
  LogDebug("CRTSPClient::Shutdown()");

  // Teardown, then shutdown, any outstanding RTP/RTCP subsessions
  if (m_session != NULL)
  {
    if (m_ourClient != NULL)
    {
      LogDebug("CRTSPClient::Shutdown(): send RTSP TEARDOWN");
      m_ourClient->sendTeardownCommand(*m_session, NULL);   // There's nothing we can do if this doesn't succeed => no response handler.
    }

    MediaSubsessionIterator iter(*m_session);
    MediaSubsession* subsession;
    while ((subsession = iter.next()) != NULL) 
    {
      Medium::close(subsession->sink);
      subsession->sink = NULL;
    }

    Medium::close(m_session);
    m_session = NULL;
  }

  // Finally, shut down our client:
  if (m_ourClient != NULL)
  {
    Medium::close(m_ourClient);
    m_ourClient = NULL;
  }

  StopBufferThread();

  m_buffer.Clear();
}

bool CRTSPClient::OpenStream(char* url)
{
  LogDebug("CRTSPClient::OpenStream()");
  if (m_env == NULL)
  {
    LogDebug("CRTSPClient::OpenStream(): environment is NULL");
    return false;
  }
	
  strcpy(m_url, url);
  if (m_ourClient != NULL)
  {
    Shutdown();
  }
  LogDebug("CRTSPClient::OpenStream(): create RTSP client");
  m_ourClient = MPRTSPClient::createNew(this, *m_env, m_url, 0/*verbosity level*/, "TSFileSource");
  if (m_ourClient == NULL) 
  {
    LogDebug("CRTSPClient::OpenStream(): failed to create RTSP client, message = %s", m_env->getResultMsg());
    return false;
  }

  // Thread has to be running so that LIVE555 can handle RTSP command
  // responses in the background.
  StartBufferThread();

  if (!InternalUpdateDuration(m_ourClient))
  {
    Shutdown();
    return false;
  }
  LogDebug("CRTSPClient::OpenStream(): duration = %d", m_duration);

  // Create a media session object from the SDP description:
  m_session = MediaSession::createNew(*m_env, m_durationDescribeResponseResultString);
  if (m_session == NULL) 
  {
    LogDebug("CRTSPClient::OpenStream(): failed to create media session from RTSP DESCRIBE response, message = %s", m_env->getResultMsg());
    Shutdown();
    return false;
  } 
  else if (!m_session->hasSubsessions()) 
  {
    LogDebug("CRTSPClient::OpenStream(): media session has no sub-sessions");
    Shutdown();
    return false;
  }

  // Then, setup the RTP sources for the session:
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  Boolean madeProgress = False;
  int desiredPortNum = 0;
  while ((subsession = iter.next()) != NULL) 
  {
    if (desiredPortNum != 0) 
    {
      subsession->setClientPortNum(desiredPortNum);
      desiredPortNum += 2;
    }

    if (!subsession->initiate(-1)) 
    {
      LogDebug("CRTSPClient::OpenStream(): failed to create receiver for sub-session, medium name = %s, codec name = %s, message = %s", subsession->mediumName(), subsession->codecName(), m_env->getResultMsg());
      continue;
    }

    LogDebug("CRTSPClient::OpenStream(): created receiver for sub-session, medium name = %s, codec name = %s, port = %d", subsession->mediumName(), subsession->codecName(), subsession->clientPortNum());
    madeProgress = True;

    if (subsession->rtpSource() != NULL) 
    {
      // Because we're saving the incoming data, rather than playing
      // it in real time, allow an especially large time threshold
      // (500 milliseconds) for reordering misordered incoming packets:
      int socketNum = subsession->rtpSource()->RTPgs()->socketNum();
      increaseReceiveBufferTo(*m_env, socketNum, 2000000);

      subsession->rtpSource()->setPacketReorderingThresholdTime(500000);  // 500 milliseconds
    }
  }

  if (!madeProgress || !SetupStreams()) 
  {
    Shutdown();
    return false;
  }
	
  // Create output files (file sinks) for each sub-session:
  iter.reset();
  while ((subsession = iter.next()) != NULL) 
  {
    if (subsession->readSource() == NULL)
    {
      continue;
    }
		
    CMemorySink* fileSink = CMemorySink::createNew(*m_env, m_buffer, 20000/*buffer size*/);
    subsession->sink = fileSink;
    if (subsession->sink == NULL) 
    {
      LogDebug("CRTSPClient::OpenStream(): failed to create file sink, message = %s", m_env->getResultMsg());
      Shutdown();
      return false;
    }

    LogDebug("CRTSPClient::OpenStream(): created file sink");

    // Set handlers and start playing.
    subsession->sink->startPlaying(*(subsession->readSource()), SubsessionAfterPlaying, subsession);
    if (subsession->rtcpInstance() != NULL) 
    {
      subsession->rtcpInstance()->setByeHandler(SubsessionByeHandler,subsession);
    }
  }
  return true;
}

void CRTSPClient::Stop()
{
  LogDebug("CRTSPClient:Stop()");
  Shutdown();
  LogDebug("CRTSPClient:Stop(): done");
}

void CRTSPClient::StartBufferThread()
{
  if (!m_isBufferThreadActive)
  {
    LogDebug("CRTSPClient::StartBufferThread()");
    StartThread();
    LogDebug("CRTSPClient::StartBufferThread(): done");
  }
}

void CRTSPClient::StopBufferThread()
{
  m_isRunning = false;
  if (!m_isBufferThreadActive)
  {
    return;
  }

  LogDebug("CRTSPClient::StopBufferThread()");
  StopThread(20000);

  m_isBufferThreadActive = false;
  LogDebug("CRTSPClient::StopBufferThread(): done");
}

long CRTSPClient::Duration()
{
  return m_duration;
}

void CRTSPClient::ThreadProc()
{
  HRESULT hr = S_OK;
  m_isBufferThreadActive = true;
  m_isRunning = true;
  ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
  LogDebug("CRTSPClient::ThreadProc(): thread started, thread ID = %d", GetCurrentThreadId());
  while (m_env != NULL && !ThreadIsStopping(1))
  {
    for (int i = 0; i < 10; ++i)
    {
      if (!m_isRunning)
      {
        break;
      }
      m_env->taskScheduler().doEventLoop(); 
    }
  }
  LogDebug("CRTSPClient::ThreadProc(): thread stopping, thread ID = %d", GetCurrentThreadId());
  m_isBufferThreadActive = false;
  return;
}

bool CRTSPClient::Play(double start, double duration)
{
  // I think this clause is here to support use of TsReader outside
  // MediaPortal. For example, cases where the graph is stopped and then
  // restarted. Within MP that never happens. The graph will be paused and
  // restarted, or completely rebuilt.
  if (m_ourClient == NULL || m_session == NULL)
  {
    char* url = new char[strlen(m_url) + 1];
    if (url != NULL)
    {
      strcpy(url, m_url);
      bool openResult = OpenStream(url);
      delete[] url;
      if (!openResult)
      {
        Shutdown();
        return false;
      }
    }
  }

  long dur = m_duration / 1000;
  if (duration > 0.0)
  {
    double remainingDuration = duration - start;
    if (remainingDuration < 0)
    {
      remainingDuration = 0;
    }
    start = dur - remainingDuration;
    if (start < 0)
    {
      start = 0;
    }
  }

  LogDebug("CRTSPClient::Play(): start = %f, duration = %f", (float)start, (float)duration);
  StartBufferThread();  // Note: thread expected to be running already. This is for "safety".
  if (!InternalPlay(start)) 
  {
    Shutdown();
    return false;
  }
  return true;
}

void CRTSPClient::Continue()
{
  if (m_ourClient != NULL && m_session != NULL)
  {
    StartBufferThread();
    if (!InternalPlay(-1.0))
    {
      StopBufferThread();
    }
  }
}

bool CRTSPClient::InternalPlay(double startPoint)
{
  LogDebug("CRTSPClient::Play()");
  if (m_ourClient != NULL && m_session != NULL)
  {
    ResetEvent(m_genericResponseEvent);
    m_ourClient->sendPlayCommand(*m_session, &CRTSPClient::OnGenericResponseReceived, startPoint);
    if (WaitForSingleObject(m_genericResponseEvent, TIMEOUT_GENERIC_RTSP_RESPONSE) == WAIT_TIMEOUT)
    {
      LogDebug("CRTSPClient::Play(): RTSP PLAY timed out");
      return false;
    }
    if (m_genericResponseResultCode != 0)
	  {
      LogDebug("CRTSPClient::Play(): RTSP PLAY failed, result code = %d, message = %s", m_genericResponseResultCode, m_env->getResultMsg());
      return false;
    }

    m_isPaused = false;
  }
  LogDebug("CRTSPClient::Play(): done");
  return true;
}

bool CRTSPClient::IsPaused()
{
  return m_isPaused;
}

bool CRTSPClient::Pause()
{
  LogDebug("CRTSPClient::Pause()");
  if (m_ourClient != NULL && m_session != NULL)
  {
    ResetEvent(m_genericResponseEvent);
    m_ourClient->sendPauseCommand(*m_session, &CRTSPClient::OnGenericResponseReceived);
    if (WaitForSingleObject(m_genericResponseEvent, TIMEOUT_GENERIC_RTSP_RESPONSE) == WAIT_TIMEOUT)
    {
      LogDebug("CRTSPClient::Pause(): RTSP PAUSE timed out");
      return false;
    }
    if (m_genericResponseResultCode != 0)
	  {
      LogDebug("CRTSPClient::Pause(): RTSP PAUSE failed, result code = %d, message = %s", m_genericResponseResultCode, m_env->getResultMsg());
      return false;
    }

    m_isPaused = true;
    StopBufferThread();
  }
  LogDebug("CRTSPClient::Pause(): done");
  return true;
}

void CRTSPClient::OnGenericResponseReceived(RTSPClient* client, int resultCode, char* resultString)
{
  LogDebug("CRTSPClient::OnGenericResponseReceived(): result code = %d", resultCode);
  if (resultString != NULL)
  {
    delete[] resultString;
  }
  CRTSPClient* rtspClient = (CRTSPClient*)((MPRTSPClient*)client)->Context();
  rtspClient->m_genericResponseResultCode = resultCode;
  SetEvent(rtspClient->m_genericResponseEvent);
}

bool CRTSPClient::UpdateDuration()
{
  //LogDebug("CRTSPClient::UpdateDuration()");
  if (m_env == NULL)
  {
    LogDebug("CRTSPClient::UpdateDuration(): environment is NULL");
    return false;
  }

  MPRTSPClient* client = MPRTSPClient::createNew(this, *m_env, m_url, 0/*verbosity level*/, "TSFileSource");
  if (client == NULL)
  {
    LogDebug("CRTSPClient::UpdateDuration(): failed to create RTSP client");
    return false;
  }

  bool result = InternalUpdateDuration(client);
  Medium::close(client);
  return result;
}

bool CRTSPClient::InternalUpdateDuration(MPRTSPClient* client)
{
  ResetEvent(m_durationDescribeResponseEvent);
  client->sendDescribeCommand(&CRTSPClient::OnDurationDescribeResponseReceived);

  // Wait for a response. Don't wait longer than the calling period (currently ~1000 ms).
  if (WaitForSingleObject(m_durationDescribeResponseEvent, 500) == WAIT_TIMEOUT)
  {
    LogDebug("CRTSPClient::UpdateDuration(): RTSP DESCRIBE timed out");
    return false;
  }

  //LogDebug("CRTSPClient::UpdateDuration(): result code = %d", m_durationDescribeResponseResultCode);
  if (m_durationDescribeResponseResultCode != 0)
	{
    LogDebug("CRTSPClient::UpdateDuration(): RTSP DESCRIBE failed, result code = %d, message = %s", m_durationDescribeResponseResultCode, m_env->getResultMsg());
    return false;
  }
  //LogDebug("CRTSPClient::UpdateDuration(): RTSP DESCRIBE response = %s", m_durationDescribeResponseResultString);

  char* rangeSection = strstr(m_durationDescribeResponseResultString, "a=range:npt=");
  if (rangeSection != NULL)
  {
    char* startString = rangeSection + strlen("a=range:npt=");
    char* endString = strstr(rangeSection, "-");
    if (endString != NULL)
    {
      endString++;
      double start = atof(startString);
      double end = atof(endString);

      //LogDebug("CRTSPClient::UpdateDuration(): range start = %f, range end = %f", start, end);
      m_duration = (end - start) * 1000.0;
      return true;
    }
  }
  return false;
}

void CRTSPClient::OnDurationDescribeResponseReceived(RTSPClient* client, int resultCode, char* resultString)
{
  //LogDebug("CRTSPClient::OnDurationDescribeResponseReceived(): result code = %d", resultCode);
  CRTSPClient* rtspClient = (CRTSPClient*)((MPRTSPClient*)client)->Context();
  rtspClient->m_durationDescribeResponseResultCode = resultCode;
  if (resultString == NULL)
  {
    rtspClient->m_durationDescribeResponseResultString[0] = NULL;
  }
  else
  {
    strncpy(rtspClient->m_durationDescribeResponseResultString, resultString, MAX_DURATION_DESCRIBE_RESPONSE_BYTE_COUNT);
    rtspClient->m_durationDescribeResponseResultString[MAX_DURATION_DESCRIBE_RESPONSE_BYTE_COUNT - 1] = NULL;
    delete[] resultString;
  }
  SetEvent(rtspClient->m_durationDescribeResponseEvent);
}