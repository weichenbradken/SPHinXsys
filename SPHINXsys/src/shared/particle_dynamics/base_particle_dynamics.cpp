/**
 * @file 	base_particle_dynamics.cpp
 * @author	Luhui Han, Chi ZHang and Xiangyu Hu
 * @version	0.1
 */
#include "base_particle_dynamics.h"
#include "base_particle_dynamics.hpp"
//=============================================================================================//
namespace SPH
{
	Real GlobalStaticVariables::physical_time_ = 0.0;
	//=============================================================================================//
	void InnerIterator(size_t number_of_particles, InnerFunctor &inner_functor, Real dt)
	{
		for (size_t i = 0; i < number_of_particles; ++i)
			inner_functor(i, dt);
	}
	//=============================================================================================//
	void InnerIterator_parallel(size_t number_of_particles, InnerFunctor &inner_functor, Real dt)
	{
		parallel_for(blocked_range<size_t>(0, number_of_particles),
			[&](const blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i < r.end(); ++i) {
				inner_functor(i, dt);
			}
		}, ap);
	}
	//=================================================================================================//
	void CellListIteratorSplitting(SplitCellLists& split_cell_lists,
		CellListFunctor& cell_list_functor, Real dt)
	{
		//forward sweeping
		for (size_t k = 0; k != split_cell_lists.size(); ++k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k];
			for (size_t l = 0; l != cell_lists.size(); ++l)
				cell_list_functor(cell_lists[l], dt);
		}

		//backward sweeping
		for (size_t k = split_cell_lists.size(); k != 0; --k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k - 1];
			for (size_t l = cell_lists.size(); l != 0; --l)
				cell_list_functor(cell_lists[l - 1], dt);
		}
	}
	//=================================================================================================//
	void CellListIteratorSplitting_parallel(SplitCellLists& split_cell_lists,
		CellListFunctor& cell_list_functor, Real dt)
	{
		//forward sweeping
		for (size_t k = 0; k != split_cell_lists.size(); ++k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k];
			parallel_for(blocked_range<size_t>(0, cell_lists.size()),
				[&](const blocked_range<size_t>& r) {
					for (size_t l = r.begin(); l < r.end(); ++l)
						cell_list_functor(cell_lists[l], dt);
				}, ap);
		}
	
		//backward sweeping
		for (size_t k = split_cell_lists.size(); k >= 1; --k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k - 1];
			parallel_for(blocked_range<size_t>(0, cell_lists.size()),
				[&](const blocked_range<size_t>& r) {
					for (size_t l = r.end(); l >= r.begin() + 1; --l) {
						cell_list_functor(cell_lists[l -1], dt);
					}
				}, ap);
		}
	}
	//=================================================================================================//
	void InnerIteratorSplitting(SplitCellLists& split_cell_lists,
		InnerFunctor &inner_functor, Real dt)
	{
		for (size_t k = 0; k != split_cell_lists.size(); ++k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k];
			for (size_t l = 0; l != cell_lists.size(); ++l)
			{
				IndexVector& particle_indexes
					= cell_lists[l]->real_particle_indexes_;
				for (size_t i = 0; i != particle_indexes.size(); ++i)
				{
					inner_functor(particle_indexes[i], dt);
				}
			}
		}
	}
	//=================================================================================================//
	void InnerIteratorSplitting_parallel(SplitCellLists& split_cell_lists,
		InnerFunctor& inner_functor, Real dt)
	{
		for (size_t k = 0; k != split_cell_lists.size(); ++k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k];
			parallel_for(blocked_range<size_t>(0, cell_lists.size()),
				[&](const blocked_range<size_t>& r) {
					for (size_t l = r.begin(); l < r.end(); ++l) {
						IndexVector& particle_indexes
							= cell_lists[l]->real_particle_indexes_;
						for (size_t i = 0; i < particle_indexes.size(); ++i)
						{
							inner_functor(particle_indexes[i], dt);
						}
					}
				}, ap);
		}
	}
	//=================================================================================================//
	void InnerIteratorSplittingSweeping(SplitCellLists& split_cell_lists,
		InnerFunctor& inner_functor, Real dt)
	{
		Real dt2 = dt * 0.5;
		//forward sweeping
		for (size_t k = 0; k != split_cell_lists.size(); ++k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k];
			for (size_t l = 0; l != cell_lists.size(); ++l)
			{
				IndexVector& particle_indexes
					= cell_lists[l]->real_particle_indexes_;
				for (size_t i = 0; i != particle_indexes.size(); ++i)
				{
					inner_functor(particle_indexes[i], dt2);
				}
			}
		}

		//backward sweeping
		for (size_t k = split_cell_lists.size(); k != 0; --k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k - 1];
			for (size_t l = 0; l != cell_lists.size(); ++l)
			{
				IndexVector& particle_indexes
					= cell_lists[l]->real_particle_indexes_;
				for (size_t i = particle_indexes.size(); i != 0; --i)
				{
					inner_functor(particle_indexes[i - 1], dt2);
				}
			}
		}
	}
	//=================================================================================================//
	void InnerIteratorSplittingSweeping_parallel(SplitCellLists& split_cell_lists,
		InnerFunctor &inner_functor, Real dt)
	{
		Real dt2 = dt * 0.5;
		//forward sweeping
		for (size_t k = 0; k != split_cell_lists.size(); ++k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k];
			parallel_for(blocked_range<size_t>(0, cell_lists.size()),
				[&](const blocked_range<size_t>& r) {
					for (size_t l = r.begin(); l < r.end(); ++l) {
						IndexVector& particle_indexes
							= cell_lists[l]->real_particle_indexes_;
						for (size_t i = 0; i < particle_indexes.size(); ++i)
						{
							inner_functor(particle_indexes[i], dt2);
						}
					}
				}, ap);
		}

		//backward sweeping
		for (size_t k = split_cell_lists.size(); k != 0; --k) {
			ConcurrentCellLists& cell_lists = split_cell_lists[k - 1];
			parallel_for(blocked_range<size_t>(0, cell_lists.size()),
				[&](const blocked_range<size_t>& r) {
				for (size_t l = r.begin(); l < r.end(); ++l) {
					IndexVector& particle_indexes
						= cell_lists[l]->real_particle_indexes_;
					for (size_t i = particle_indexes.size(); i != 0; --i)
					{
						inner_functor(particle_indexes[i - 1], dt2);
					}
				}
			}, ap);
		}
	}
	//=============================================================================================//
}
//=============================================================================================//