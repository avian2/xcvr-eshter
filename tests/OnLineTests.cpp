#include "CppUTest/CommandLineTestRunner.h"

#include "SpectrumSensor.hpp"
#include "DeviceImp.hpp"

TEST_GROUP(SpectrumSensorTestGroup)
{
};

TEST(SpectrumSensorTestGroup, TestError)
{
	VESNA::SpectrumSensor ss("/dev/ttyUSB0");

	ss.comm->write("invalid-command\n");

	std::string what;
	int t = 0;
	try {
		ss.wait_for_ok();
	} catch(VESNA::SpectrumSensorException e) {
		what = e.what();
		t = 1;
	}

	CHECK(!what.compare("error: unknown command: invalid-command"));
	CHECK(t == 1);
}

TEST(SpectrumSensorTestGroup, TestSpectrumSensor)
{
	VESNA::SpectrumSensor ss("/dev/ttyUSB0");
}

TEST(SpectrumSensorTestGroup, TestGetConfigList)
{
	VESNA::SpectrumSensor ss("/dev/ttyUSB0");

	VESNA::ConfigList* cl = ss.get_config_list();

	CHECK(cl != NULL);

	VESNA::DeviceConfig* c = cl->get_config(0, 0);

	CHECK(c != NULL);

	delete cl;
}

TEST(SpectrumSensorTestGroup, TestSelectSweepChannel)
{
	VESNA::SpectrumSensor ss("/dev/ttyUSB0");
	VESNA::ConfigList* cl = ss.get_config_list();
	VESNA::DeviceConfig* c = cl->get_config(0, 0);
	VESNA::SweepConfig* sc = c->get_sample_config(c->base, 100);

	ss.select_sweep_channel(sc);

	delete sc;
	delete cl;
}

int test_cb_cnt;
const unsigned nsamples = 25000;

bool test_cb(const VESNA::SweepConfig* sc, const VESNA::TimestampedData* samples, void* priv)
{
	CHECK(nsamples == samples->data.size());
	std::vector<VESNA::data_t>::const_iterator i = samples->data.begin();
	for(; i != samples->data.end(); ++i) {
		CHECK(*i >= 0.0);
		CHECK(*i < 4096.0);
	}

	test_cb_cnt++;
	if(test_cb_cnt >= 2) {
		return false;
	} else {
		return true;
	}
}

TEST(SpectrumSensorTestGroup, TestSampleRun)
{
	VESNA::SpectrumSensor ss("/dev/ttyUSB0");
	VESNA::ConfigList* cl = ss.get_config_list();

	VESNA::DeviceConfig* c = cl->get_config(0, 2);
	VESNA::SweepConfig* sc = c->get_sample_config(c->base, nsamples);

	test_cb_cnt = 0;
	ss.sample_run(sc, test_cb, NULL);

	CHECK_EQUAL(2, test_cb_cnt);

	delete sc;
	delete cl;
}

TEST_GROUP(DeviceImpTestGroup)
{
};

class TestReceiver : public Transceiver::I_ReceiveDataPush
{
	public:
		int sample_count;
		boost::mutex sample_count_m;

		void pushBBSamplesRx(Transceiver::BBPacket* thePushedPacket,
				Transceiver::Boolean endOfBurst) {
			boost::unique_lock<boost::mutex> lock(sample_count_m);
			sample_count += thePushedPacket->SampleNumber;
		};
		TestReceiver() : sample_count(0) {};
		~TestReceiver() {};
};

TEST(DeviceImpTestGroup, TestCreateRXProfile)
{
	TestReceiver rx;
	VESNA::SpectrumSensor ss("/dev/ttyUSB0");
	DeviceImp di(&rx, &ss);

	Transceiver::ReceiveCycleProfile profile;
	profile.ReceiveStartTime.discriminator = Transceiver::immediateDiscriminator;
	profile.ReceiveStopTime.discriminator = Transceiver::undefinedDiscriminator;
	profile.PacketSize = 1024;
	profile.TuningPreset = 2;
	profile.CarrierFrequency = 700e6;

	Transceiver::ULong i = di.receiveChannel.createReceiveCycleProfile(
		profile.ReceiveStartTime,
		profile.ReceiveStopTime,
		profile.PacketSize,
		profile.TuningPreset,
		profile.CarrierFrequency);

	while(1) {
		{
			boost::unique_lock<boost::mutex> lock(rx.sample_count_m);
			if(rx.sample_count > 0) {
				break;
			}
		}
	}

	Transceiver::Time stop(Transceiver::immediateDiscriminator);
	di.receiveChannel.setReceiveStopTime(i, stop);

	di.receiveChannel.wait();

	CHECK(rx.sample_count > 0);
	CHECK(rx.sample_count % 1024 == 0);
}
