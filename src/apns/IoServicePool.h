
#ifndef IoServicePool_H
#define IoServicePool_H

#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

class IoServicePool: private boost::noncopyable
{
public:
  explicit IoServicePool(std::size_t pool_size);
  ~IoServicePool();

  void Run();
  void Stop();

  boost::asio::io_service* GetIoService();

private:
  typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
  typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;


  std::vector<io_service_ptr> io_services_;
  std::vector<work_ptr> work_;
  std::size_t next_io_service_;
};

#endif // IoServicePool_H
