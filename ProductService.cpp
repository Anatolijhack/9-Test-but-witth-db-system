#include "ProductService.h"

std::string ProductService::get_products()
{
	return repository.get_products();
}

std::string ProductService::get_product(int id)
{
	return repository.get_product(id);
}

std::string ProductService::add_product(const std::string& type, const std::string& name, double cost, double price, int stock)
{
	return repository.add_product(type, name, cost, price, stock);
}

std::string ProductService::update_product(int id, double price, int stock)
{
	return repository.update_product(id, price, stock);
}

std::string ProductService::delete_product(int id)
{
	return repository.delete_product(id);
}
