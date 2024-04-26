#include "input_reader.h"

#include <algorithm>
#include <iterator>

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
Coordinates ParseCoordinates(std::string_view str) {
  static const double nan = std::nan("");

  auto not_space = str.find_first_not_of(' ');
  auto comma = str.find(',');

  if (comma == str.npos) {
    return {nan, nan};
  }

  auto not_space2 = str.find_first_not_of(' ', comma + 1);

  double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
  double lng = std::stod(std::string(str.substr(not_space2)));

  return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
  const auto start = string.find_first_not_of(' ');
  if (start == string.npos) {
    return {};
  }
  return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
  std::vector<std::string_view> result;

  size_t pos = 0;
  while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
    auto delim_pos = string.find(delim, pos);
    if (delim_pos == string.npos) {
      delim_pos = string.size();
    }
    if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
      result.push_back(substr);
    }
    pos = delim_pos + 1;
  }

  return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
  if (route.find('>') != route.npos) {
    return Split(route, '>');
  }

  auto stops = Split(route, '-');
  std::vector<std::string_view> results(stops.begin(), stops.end());
  results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

  return results;
}

std::unordered_map<std::string, int> ParseDistance(std::string_view line, TransportCatalogue &catalogue) {
  std::unordered_map<std::string, int> result;
  auto parts = Split(line, ',');
  for (auto& part: parts) {
    part = Trim(part);
    auto space_pos = part.find(' ');
    if (space_pos == part.npos) return {};
    auto to_pos = part.find_first_not_of(' ', space_pos);
    if (to_pos == part.npos) return {};
    auto after_to_space = part.find_first_not_of(' ', to_pos + 2);
    if (after_to_space == part.npos) return {};
    int metres = stoi(std::string(part.substr(0, space_pos - 1)));
    auto stop = part.substr(after_to_space);
    if (catalogue.FindStop(stop)) {
      result[std::string(stop)] = metres;
    }
  }
  return result;
}

CommandDescription ParseCommandDescription(std::string_view line) {
  auto colon_pos = line.find(':');
  if (colon_pos == line.npos) {
    return {};
  }

  auto space_pos = line.find(' ');
  if (space_pos >= colon_pos) {
    return {};
  }

  auto not_space = line.find_first_not_of(' ', space_pos);
  if (not_space >= colon_pos) {
    return {};
  }
  std::string command = std::string(Trim(line.substr(0, space_pos)));
  std::string id = std::string(Trim(line.substr(not_space, colon_pos - not_space)));
  std::string description = std::string(Trim(line.substr(colon_pos + 1)));

  if (command == "Bus") return {command, id, description, ""};

  auto comma = description.find_first_of(',');
  if (comma == description.npos) {
    return {};
  }

  comma = description.find_first_of(',', comma + 1);
  std::string coordinates = std::string(Trim(description.substr(0, comma)));
  if (comma == description.npos) return {command, id, coordinates, ""};
  else {
    std::string stop_distance = std::string(Trim(description.substr(comma + 1)));
    return {command, id, coordinates, stop_distance};
  }
}

void InputReader::ParseLine(std::string_view line) {
  auto command_description = ParseCommandDescription(line);
  if (command_description) {
    commands_.push_back(std::move(command_description));
  }
}


bool CommandComparator(CommandDescription lhs, CommandDescription rhs) {
  return lhs.command.size() > rhs.command.size();
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue &catalogue) const {
  auto commands = commands_;
  std::sort(commands.begin(), commands.end(), CommandComparator);
  int stop_commands = 0;
  std::deque<std::string> stop_names;
  for (auto &command: commands) {
    if (command.command == "Stop") {
      ++stop_commands;
      stop_names.push_back(command.id);
      catalogue.AddStop(std::move(command.id), ParseCoordinates(command.coordinates));
    }
  }
  for (int i = 0; i < stop_commands; ++i) {
    auto distances = ParseDistance(commands[i].distances, catalogue);
    if (distances.empty()) continue;
    for (const auto& [stop_name, num] : distances) {
      catalogue.SetDistance(stop_names[i], stop_name, num);
    }
  }
  for (int i = stop_commands; i < commands.size(); ++i) {
    std::vector<std::string_view> stops = ParseRoute(commands[i].coordinates);
    catalogue.AddBus(std::move(commands[i].id), stops);
  }
}

void RunInput(std::istream &in, TransportCatalogue &catalogue) {
  int base_request_count;
  in >> base_request_count >> std::ws;
  {
    InputReader reader;
    for (int i = 0; i < base_request_count; ++i) {
      std::string line;
      getline(in, line);
      reader.ParseLine(line);
    }
    reader.ApplyCommands(catalogue);
  }
}
