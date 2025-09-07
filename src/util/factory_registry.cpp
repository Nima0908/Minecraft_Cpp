#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <functional> 

namespace mc::factory {
    template <typename Base, typename Key, template <typename ...> typename Ptr, typename ... args>
      class FlexibleFactoryRegistry {};
}
