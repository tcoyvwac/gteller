#include <iostream>
#include <vector>
#define _USE_MATH_DEFINES
#include <cmath>
#include <optional>
#include <algorithm>
#include <array>
#include <string>
#include <thread>
#include <atomic>
#include <pcaudiolib/audio.h>

int main()
{
  audio_object* const ao=create_audio_device_object(0, "teller", "");
  if(!ao) {
    std::cerr<<"Unable to open audio file "<<std::endl;
    return 0;
  }

  if(const auto res = audio_object_open(ao, AUDIO_OBJECT_FORMAT_S16LE, 44100, 1); res < 0) {
    std::cerr<<"Error opening audio: "<<audio_object_strerror(ao, res)<<std::endl;
  }

  std::optional<std::atomic<int64_t>> counter = 0;
  const auto player = [&, data = std::vector<int16_t>(44100), ourcounter = int64_t{}]() mutable {
    while(counter) {
      data.clear();
      constexpr auto gen_sinewave_t = [data = std::array<int16_t, 250>{}]() mutable {
        std::generate(std::begin(data), std::end(data), [n=0] () mutable { return 20000 * sin((n++/44100.0) * 500 * 2 * M_PI); });
        return data;
      };
      const auto gen_waveform = [&](auto generator) {
        const auto waveform = generator();    // A callable which returns a [sinewave, nullwave] data-array.
        std::move(std::begin(waveform), std::end(waveform), std::back_inserter(data));
        return data;
      };
      const auto adjusted_counter = [&] { return ((*counter - (ourcounter++)) > 1000) ? int64_t{*counter} : ourcounter;};
      std::tie(data, ourcounter) = (ourcounter < counter)
                                   ? std::tuple{ gen_waveform(gen_sinewave_t), adjusted_counter()}
                                   : std::tuple{ gen_waveform([]{ return std::array<int16_t, 150>{}; }), ourcounter};
      audio_object_write(ao, std::data(data), std::size(data) * sizeof(decltype(data)::value_type));
    }
  };

  std::thread athread(player);
  for(std::string line; getline(std::cin, line); (*counter)++);
  counter = std::nullopt;
  athread.join();
  std::this_thread::sleep_for(std::chrono::seconds(1));
}
