﻿#pragma once

#include <communication/message/Message.h>

namespace balancedbanana {
    namespace communication {

		class AuthResultMessage : public Message {
		private:
			unsigned long status;


		public:
			//Gibt den Authentfication status zurück 0 falls erfolgreich sonst ungleich 0
			unsigned long getStatus();

			virtual void process(const std::shared_ptr<MessageProcessor>& mp);

		};

	}
}