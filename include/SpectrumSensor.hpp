#ifndef HAVE_SPECTRUMSENSOR_H
#define HAVE_SPECTRUMSENSOR_H

#include "serial/serial.h"
#include "boost/function.hpp"

namespace VESNA {

typedef long long ch_t;
typedef long long hz_t;
typedef long long ms_t;

typedef double data_t;

class SweepConfig;

class SpectrumSensorException: public std::exception
{
	public:
		SpectrumSensorException(const char *w) : what_(w) {};
		SpectrumSensorException(const std::string &w) : what_(w) {};
		~SpectrumSensorException() throw() {};

		const char *what() const throw();
	private:

		std::string what_;
};

class Device
{
	public:
		int id;
		std::string name;
		bool supports_sampling;

		Device(int id_, const std::string &name_) :
			id(id_), name(name_), supports_sampling(false) {};
};

class DeviceConfig
{
	public:
		int id;
		std::string name;
		Device *device;

		hz_t base;
		hz_t spacing;
		hz_t bw;
		ch_t num;
		ms_t time;

		DeviceConfig(int id_, const std::string &name_, Device* device_) :
			id(id_), name(name_), device(device_) {};

		hz_t ch_to_hz(ch_t ch);
		ch_t hz_to_ch(hz_t hz);

		hz_t get_start_hz();
		hz_t get_stop_hz();

		bool covers(hz_t start_hz, hz_t stop_hz);

		SweepConfig* get_sample_config(hz_t hz, int nsamples);
};

class ConfigList
{
	public:
		ConfigList() {};
		void parse(std::vector<std::string> lines);

		DeviceConfig* get_config(int device_id, int config_id);

		int get_config_num();
		int get_device_num();

	private:
		std::vector<DeviceConfig> configs;
		std::vector<Device> devices;
};

class SweepConfig
{
	public:
		DeviceConfig* config;
		ch_t start_ch;
		ch_t stop_ch;
		ch_t step_ch;
		int nsamples;

		SweepConfig() {};
		SweepConfig(DeviceConfig* config_, ch_t start_ch_, ch_t stop_ch_, ch_t step_ch_, 
				int nsamples_) :
			config(config_), start_ch(start_ch_), stop_ch(stop_ch_), step_ch(step_ch_),
			nsamples(nsamples_) {};
};

class TimestampedData
{
	public:
		double timestamp;
		ch_t channel;

		std::vector<data_t> data;

		bool parse(std::string s, int ch_num = -1);
};

typedef boost::function<bool (const VESNA::SweepConfig* sc, const VESNA::TimestampedData* samples)> sample_run_cb_t;

class I_SpectrumSensor
{
	public:
		virtual ConfigList* get_config_list() = 0;
		virtual void sample_run(const SweepConfig* sc, sample_run_cb_t cb) = 0;
};

class SpectrumSensor : public I_SpectrumSensor
{
	public:
		serial::Serial *comm;

		SpectrumSensor(const std::string &port);
		~SpectrumSensor();

		ConfigList* get_config_list();
		void sample_run(const SweepConfig* sc, sample_run_cb_t cb);

		void select_sweep_channel(const SweepConfig* sc);
		void wait_for_ok();
};

};

#endif
