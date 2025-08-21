#pragma once

namespace StartupUtils {

    void getWiiUSerialId();

    void disclaimer();

    void addInitMessage(const char *newMessage);
    void resetMessageList();
    void addInitMessageWithIcon(const char *newMessage);


} // namespace StartupUtils