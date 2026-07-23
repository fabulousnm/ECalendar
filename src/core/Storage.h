

//用户数据储存的JSON接口



#ifndef ECALENDER_STORAGE_H
#define ECALENDER_STORAGE_H

#include <string>
#include <vector>
#include "core/Task.h"
#include "core/User.h"

class Storage {
public:
   //储存任务
    static bool saveTasks(const std::string& filename, const std::vector<Task>& tasks);

   //加载任务
    static std::vector<Task> loadTasks(const std::string& filename);

   //储存用户信息
    static bool saveUsers(const std::string& filename, const std::vector<User>& users);

  //加载用户信息
    static std::vector<User> loadUsers(const std::string& filename);

   //同时保存两个（包裹函数）
    static bool saveAll(const std::string& filename,
                        const std::vector<Task>& tasks,
                        const std::vector<User>& users);

    //同时加载两个
    static bool loadAll(const std::string& filename,
                        std::vector<Task>& outTasks,
                        std::vector<User>& outUsers);
};

#endif // ECALENDER_STORAGE_H

