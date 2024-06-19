# Имя компилятора
CXX = clang++
# Опции компиляции
CXXFLAGS = -Wall -Wextra -std=c++17

# Имя исполняемого файла
TARGET = usb_nicksec
# Исходные файлы
SOURCES = main.cpp usb_monitor.cpp device_manager.cpp user_management.cpp totp.cpp logger.cpp
# Объектные файлы (получаемые в результате компиляции)
OBJECTS = $(SOURCES:.cpp=.o)

# Цель по умолчанию (собираем исполняемый файл)
all: $(TARGET)

# Команда компиляции исходных файлов в объектные
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Команда компоновки объектных файлов в исполняемый файл
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) -ludev -lgcrypt -lcrypt -lqrencode -lssl -lcrypto

# Чистка (удаление объектных файлов и исполняемого файла)
clean:
	rm -f $(OBJECTS) $(TARGET)
