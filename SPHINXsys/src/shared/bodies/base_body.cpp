/**
 * @file 	base_body.cpp
 * @brief 	Here, Functions belong to BaseBody, RealBody and FictitiousBody are given.
 * @author	hi ZHang and Xiangyu Hu
 * @version	0.1
 * 			0.2.0
 * 			Cell splitting algorithm are added. 
 * 			Chi Zhang
 */
#include "base_body.h"
#include "sph_system.h"
#include "in_output.h"
#include "base_particles.h"
#include "all_kernels.h"
#include "mesh_cell_linked_list.h"
//=================================================================================================//
namespace SPH
{
	//=================================================================================================//
	SPHBody::SPHBody(SPHSystem &sph_system, string body_name,
		int refinement_level, Real smoothing_length_ratio, ParticleGenerator* particle_generator) : 
		sph_system_(sph_system), body_name_(body_name), newly_updated_(true),
		body_lower_bound_(0), body_upper_bound_(0), prescribed_body_bounds_(false),
		refinement_level_(refinement_level), particle_generator_(particle_generator),
		body_shape_(NULL)
	{	
		sph_system_.addABody(this);
		particle_spacing_ 	= RefinementLevelToParticleSpacing();
		smoothing_length_ = particle_spacing_ * smoothing_length_ratio;
		kernel_ 			= GenerateAKernel(smoothing_length_);
		mesh_cell_linked_list_
							= new MeshCellLinkedList(this, sph_system.lower_bound_,
									sph_system_.upper_bound_, kernel_->GetCutOffRadius());
		size_t number_of_split_cell_lists = powern(3, Vecd(0).size());
		/** I will use concurrent vector here later after tests. */
		split_cell_lists_.resize(number_of_split_cell_lists);
	}
	//=================================================================================================//
	void  SPHBody::getSPHSystemBound(Vecd& system_lower_bound, Vecd& system_uppwer_bound) 
	{
		system_lower_bound = sph_system_.lower_bound_;
		system_uppwer_bound = sph_system_.upper_bound_;
	}
	//=================================================================================================//
	Real SPHBody::RefinementLevelToParticleSpacing()
	{
		return sph_system_.particle_spacing_ref_	
			/ powern(2.0, refinement_level_);
	}
	//=================================================================================================//
	Kernel* SPHBody::GenerateAKernel(Real smoothing_length)
	{
		return new KernelWendlandC2(smoothing_length);
	}
	//=================================================================================================//
	void SPHBody::ReplaceKernelFunction(Kernel* kernel)
	{
		delete kernel_;
		kernel_ = kernel;
		mesh_cell_linked_list_->reassignKernel(kernel);
	}
	//=================================================================================================//
	string SPHBody::GetBodyName()
	{
		return body_name_;
	}
	//=================================================================================================//
	SPHSystem& SPHBody::getSPHSystem()
	{
		return sph_system_;
	}
	//=================================================================================================//
	void SPHBody::assignBaseParticle(BaseParticles* base_particles)
	{
		base_particles_ = base_particles;
		mesh_cell_linked_list_->assignBaseParticles(base_particles);
	}
	//=================================================================================================//
	void SPHBody::allocateConfigurationMemoriesForBodyBuffer()
	{
		for (size_t i = 0; i < body_relations_.size(); i++)
		{
			body_relations_[i]->updateConfigurationMemories();
		}
	}
	//=================================================================================================//
	void SPHBody::findBodyDomainBounds(Vecd& lower_bound, Vecd& upper_bound)
	{
		if(!prescribed_body_bounds_) 
		{
			body_shape_->findBounds(lower_bound, upper_bound);
		}
		else 
		{
			lower_bound = body_lower_bound_;
			upper_bound = body_upper_bound_;
		}
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToVtuFile(ofstream &output_file)
	{
		base_particles_->writeParticlesToVtuFile(output_file);
		newly_updated_ = false;
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToPltFile(ofstream &output_file)
	{
		if (newly_updated_) base_particles_->writeParticlesToPltFile(output_file);
		newly_updated_ = false;
	}
	//=================================================================================================//
	void SPHBody::writeParticlesToXmlForRestart(std::string &filefullpath)
	{
		base_particles_->writeParticlesToXmlForRestart(filefullpath);
	}
	//=================================================================================================//
	void SPHBody::readParticlesFromXmlForRestart(std::string &filefullpath)
	{
		base_particles_->readParticleFromXmlForRestart(filefullpath);
	}
	//=================================================================================================//
	void SPHBody::writeToXmlForReloadParticle(std::string &filefullpath)
	{
		base_particles_->writeToXmlForReloadParticle(filefullpath);
	}
	//=================================================================================================//
	void SPHBody::readFromXmlForReloadParticle(std::string &filefullpath)
	{
		base_particles_->readFromXmlForReloadParticle(filefullpath);
	}
	//=================================================================================================//
	SPHBody* SPHBody::pointToThisObject()
	{
		return this;
	}
	//=================================================================================================//
	RealBody::RealBody(SPHSystem &sph_system, string body_name,
		int refinement_level, Real smoothing_length_ratio, ParticleGenerator* particle_generator)
	: SPHBody(sph_system, body_name, refinement_level, smoothing_length_ratio, particle_generator)
	{
		sph_system.addARealBody(this);

		mesh_cell_linked_list_->allocateMeshDataMatrix();
	}
	//=================================================================================================//
	void RealBody::allocateMemoryCellLinkedList()
	{
		mesh_cell_linked_list_->allocateMeshDataMatrix();
	}
	//=================================================================================================//
	void RealBody::updateCellLinkedList()
	{
		mesh_cell_linked_list_->UpdateCellLists();
	}
	//=================================================================================================//
	RealBody* RealBody::pointToThisObject()
	{
		return this;
	}
	//=================================================================================================//
	FictitiousBody::FictitiousBody(SPHSystem &system, string body_name, int refinement_level,
		Real smoothing_length_ratio, ParticleGenerator* particle_generator)
	: SPHBody(system, body_name, refinement_level, smoothing_length_ratio, particle_generator)
	{
		system.addAFictitiousBody(this);
	}
	//=================================================================================================//
	void FictitiousBody::allocateMemoryCellLinkedList()
	{
		/** do nothing here. */;
	}
	//=================================================================================================//
	void FictitiousBody::updateCellLinkedList()
	{
		/** do nothing here. */;
	}
	//=================================================================================================//
	FictitiousBody* FictitiousBody::pointToThisObject()
	{
		return this;
	}
	//=================================================================================================//
	void BodyPartByParticle::tagAParticle(size_t particle_index)
	{
		body_part_particles_.push_back(particle_index);
		body_->base_particles_->is_sortable_[particle_index] = false;
	}
	//=================================================================================================//
	void BodyPartByParticle::tagBodyPart()
	{
		BaseParticles* base_particles = body_->base_particles_;
		for (size_t i = 0; i < body_->number_of_particles_; ++i)
		{
			if (body_part_shape_->checkContain(base_particles->pos_n_[i])) tagAParticle(i);
		}
	}
	//=================================================================================================//
	BodySurface::BodySurface(SPHBody* body)
		: BodyPartByParticle(body, "Surface")
	{
		tagBodyPart();
	}	
	//=================================================================================================//
	void BodySurface::tagBodyPart()
	{
		for (size_t i = 0; i < body_->number_of_particles_; ++i)
		{
			Real phi = body_->body_shape_->findSignedDistance(body_->base_particles_->pos_n_[i]);
			if (fabs(phi) < body_->particle_spacing_) tagAParticle(i);
		}
		std::cout << "Number of surface particles : " << body_part_particles_.size() << std::endl;
	}
	//=================================================================================================//
	BodySurfaceLayer::BodySurfaceLayer(SPHBody* body, Real layer_thickness)
		: BodyPartByParticle(body, "InnerLayers"), layer_thickness_(layer_thickness)
	{
		tagBodyPart();
	}
	//=================================================================================================//
	void BodySurfaceLayer::tagBodyPart()
	{
		for (size_t i = 0; i < body_->number_of_particles_; ++i)
		{
			Vecd position_i = body_->base_particles_->pos_n_[i];
			Real distance = (body_->body_shape_->findClosestPoint(position_i) - position_i).norm();
			if (distance < body_->particle_spacing_* layer_thickness_) tagAParticle(i);
		}
		std::cout << "Number of inner layers particles : " << body_part_particles_.size() << std::endl;
	}
	//=================================================================================================//
	NearBodySurface::NearBodySurface(SPHBody* body)
		: BodyPartByCell(body, "NearBodySurface")
	{
		tagBodyPart();
	}
	//=================================================================================================//
}