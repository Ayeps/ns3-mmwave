/*
 * mmwave-propagation-loss-model.cc
 *
 *  Created on: Oct 28, 2015
 *      Author: mengleizhang
 */

#include "mmwave-propagation-loss-model.h"
#include <ns3/log.h>
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <ns3/simulator.h>

using namespace ns3;


// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (MmWavePropagationLossModel);

TypeId
MmWavePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWavePropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .AddConstructor<MmWavePropagationLossModel> ()
    .AddAttribute ("Frequency",
                   "The carrier frequency (in Hz) at which propagation occurs  (default is 28 GHz).",
                   DoubleValue (60e9),
                   MakeDoubleAccessor (&MmWavePropagationLossModel::SetFrequency,
                                       &MmWavePropagationLossModel::GetFrequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinLoss",
                   "The minimum value (dB) of the total loss, used at short ranges. Note: ",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&MmWavePropagationLossModel::SetMinLoss,
                                       &MmWavePropagationLossModel::GetMinLoss),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

MmWavePropagationLossModel::MmWavePropagationLossModel ()
{
  m_channelScenarioMap.clear ();
}

void
MmWavePropagationLossModel::SetMinLoss (double minLoss)
{
  m_minLoss = minLoss;
}
double
MmWavePropagationLossModel::GetMinLoss (void) const
{
  return m_minLoss;
}

void
MmWavePropagationLossModel::SetFrequency (double frequency)
{
  //NS_ASSERT (m_frequency = frequency);
  m_frequency = frequency;
  static const double C = 299792458.0; // speed of light in vacuum
  m_lambda = C / frequency;
}

double
MmWavePropagationLossModel::GetFrequency (void) const
{
  return m_frequency;
}

double
MmWavePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
  /*
   * Millimeter wave LOS/NLOS path loss equation:
   * where alpha,beta and Xi are in dB.
   *
   * PL(dB) = alpha + beta * 10 * log10(L) + Xi
   *
   * PL: Path Loss (dB)
   * d: distance (m)
   * Xi: Xi~N(0,sigma^2)
   *
   */
  double distance = a->GetDistanceFrom (b);
  if (distance < 3*m_lambda)
    {
	  //NS_LOG_WARN ("distance not within the far field region => inaccurate propagation loss value");
    }
  if (distance <= 0)
    {
      return txPowerDbm - m_minLoss;
    }

  double aOut = 0.0334;
  double bOut = 5.2;
  double aLos = 0.0149;
  double POut = fmax (0, 1-exp(((-1)*aOut*distance)+bOut));
  double PLos = (1-POut)*exp((-1)*aLos*distance);
  double PNlos = 1-POut - PLos;
  double alpha, beta, sigma;

  channelScenarioMap_t::const_iterator it;
  it = m_channelScenarioMap.find(std::make_pair(a,b));
  if (it == m_channelScenarioMap.end ())
    {
	  channelScenario scenario;
	  Ptr<UniformRandomVariable> uniformVariable = CreateObject <UniformRandomVariable> ();
	  Ptr<NormalRandomVariable> normalVariable = CreateObject <NormalRandomVariable> ();
	  normalVariable->SetAntithetic(true);
	  double PRef = uniformVariable->GetValue(0,1);

	  if (PRef < PLos)
	    {
		  scenario.m_channelScenario = 'l';
		  sigma = 5.8;
	    }
	  else if (PRef < (1-POut))
	    {
		  scenario.m_channelScenario = 'n';
		  if (m_frequency ==28e9)
		  {
			  sigma = 8.7;
		  }
		  else if (m_frequency == 73e9)
		  {
			  sigma = 7.7;
		  }
		  else
		  {
			  NS_FATAL_ERROR ("The model currently supports only 28 GHz and 73 GHz carrier frequencies.");
		  }
	    }
	  else
	    {
		  scenario.m_channelScenario = 'o';
		  return (txPowerDbm - 500.00);
	    }
	  scenario.m_shadowing = normalVariable->GetValue(0,sigma);
	  m_channelScenarioMap.insert (std::make_pair(std::make_pair (a,b), scenario));
	  m_channelScenarioMap.insert (std::make_pair(std::make_pair (b,a), scenario));
	  it = m_channelScenarioMap.find(std::make_pair(a,b));
    }
  switch ((*it).second.m_channelScenario)
    {
	  case 'l':
	    {
	      if (m_frequency == 28e9)
	        {
			  alpha = 61.4;
			  beta = 2;
	        }
	      else if (m_frequency == 73e9)
	        {
			  alpha = 69.8;
			  beta = 2;
	        }
		  else
		  {
			  NS_FATAL_ERROR ("Other frequency is not impletmented.");
		  }
		  break;
	    }
	  case 'n':
	    {
		  if (m_frequency == 28e9)
			{
			  alpha = 72.0;
			  beta = 2.92;
			}
		  else if (m_frequency == 73e9)
			{
			  alpha = 82.7;
			  beta = 2.69;
			}
		  else
		  {
			  NS_FATAL_ERROR ("Other frequency is not impletmented.");
		  }
		  break;
	    }
	  case 'o':
	    {
		  return (txPowerDbm - 500.00);
		  break;
	    }
	  default:
		  NS_FATAL_ERROR ("Programming Error.");
    }

  //NS_LOG_DEBUG ("distance=" << distance<< ", scenario=" << (*it).second.m_channelScenario<<", shadowing"<<(*it).second.m_shadowing);
  double lossDb = alpha + beta * 10 * log10(distance) + (*it).second.m_shadowing;
  NS_LOG_UNCOND ("time="<<Simulator::Now ().GetNanoSeconds () / (double) 1e9<<'\t'<<"POut="<<POut<<'\t'<<"PLos="<<PLos<<'\t'<<"PNlos="<<PNlos<<'\t'<<"lossDb="<<lossDb);
  return txPowerDbm - std::max (lossDb, m_minLoss);
}

int64_t
MmWavePropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

void
MmWavePropagationLossModel::UpDataScenarioMap ()
{
	m_channelScenarioMap.clear ();
	Simulator::Schedule(Seconds(1), &MmWavePropagationLossModel::UpDataScenarioMap,this);
}
