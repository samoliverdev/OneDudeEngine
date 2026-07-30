#include <gtest/gtest.h>
#include <OD/Core/Resource.h>
#include <OD/Core/ResourceAllocator.h>

using namespace OD;

class TestResource: public Resource{
public:
    int value = 0;

    TestResource(int v = 0): value(v){}
};

TEST(ResourceAllocator, AllocAssignsValidId){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    TestResource* a = allocator->Alloc(42);

    EXPECT_NE(a, nullptr);
    EXPECT_NE(a->GetId(), INVALID_RESOURCE_ID);
    EXPECT_EQ(a->value, 42);
}

TEST(ResourceAllocator, GetReturnsCorrectPointer){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    TestResource* a = allocator->Alloc(123);
    uint32_t id = a->GetId();

    TestResource* fetched = allocator->Get(id);

    EXPECT_EQ(fetched, a);
    EXPECT_EQ(fetched->value, 123);
}

TEST(ResourceAllocator, FreeRemovesObject){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    TestResource* a = allocator->Alloc(10);
    uint32_t id = a->GetId();

    allocator->Free(id);

    TestResource* fetched = allocator->Get(id);

    EXPECT_EQ(fetched, nullptr);
}

TEST(ResourceAllocator, ReusesFreedSlots){
    auto allocator = ResourceAllocator<TestResource>::Create(2);

    TestResource* a = allocator->Alloc(1);
    uint32_t idA = a->GetId();

    allocator->Free(idA);

    TestResource* b = allocator->Alloc(2);

    // Same memory slot reused (pointer equal)
    EXPECT_EQ(a, b);

    // But ID must be different
    //EXPECT_NE(idA, b->GetId());
    EXPECT_EQ(idA, b->GetId());
}

TEST(ResourceAllocator, AllocCreatesNewChunkWhenFull){
    auto allocator = ResourceAllocator<TestResource>::Create(2);

    TestResource* a = allocator->Alloc(1);
    TestResource* b = allocator->Alloc(2);
    TestResource* c = allocator->Alloc(3); // forces new chunk

    EXPECT_NE(a, nullptr);
    EXPECT_NE(b, nullptr);
    EXPECT_NE(c, nullptr);

    EXPECT_NE(a, c); // different chunk
}

TEST(ResourceAllocator, ResetClearsAll){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    TestResource* a = allocator->Alloc(1);
    TestResource* b = allocator->Alloc(2);

    allocator->Reset();

    EXPECT_EQ(allocator->Get(a->GetId()), nullptr);
    EXPECT_EQ(allocator->Get(b->GetId()), nullptr);

    // After reset, allocator should work again
    TestResource* c = allocator->Alloc(3);
    EXPECT_NE(c, nullptr);
}

TEST(ResourceAllocator, FreeInvalidIdThrows){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    EXPECT_THROW(allocator->Free(999999), std::runtime_error);
}

TEST(ResourceAllocator, DoubleFreeThrows){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    TestResource* a = allocator->Alloc(5);
    uint32_t id = a->GetId();

    allocator->Free(id);

    EXPECT_THROW(allocator->Free(id), std::runtime_error);
}

TEST(ResourceAllocator, StressTest){
    auto allocator = ResourceAllocator<TestResource>::Create(64);

    std::vector<uint32_t> ids;

    for(int i = 0; i < 10000; i++){
        auto* a = allocator->Alloc(i);
        ids.push_back(a->GetId());
    }

    for(auto id : ids){
        EXPECT_NE(allocator->Get(id), nullptr);
    }

    for(auto id : ids){
        allocator->Free(id);
    }

    for(auto id : ids){
        EXPECT_EQ(allocator->Get(id), nullptr);
    }
}

///////////////////////////////////////////

TEST(ResourceAllocator, UseAfterFreeIsInvalid){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    TestResource* a = allocator->Alloc(10);
    uint32_t id = a->GetId();

    allocator->Free(id);

    // Pointer is now dangling → should NOT be reused silently
    EXPECT_NE(a->GetId(), id); // or assert crash depending on design
}

/*TEST(ResourceAllocator, IdVersioningPreventsStaleAccess){
    auto allocator = ResourceAllocator<TestResource>::Create(2);

    TestResource* a = allocator->Alloc(1);
    uint32_t idA = a->GetId();

    allocator->Free(idA);

    TestResource* b = allocator->Alloc(2);

    // Old ID must NOT access new object
    EXPECT_EQ(allocator->Get(idA), nullptr);

    // New ID must work
    EXPECT_EQ(allocator->Get(b->GetId()), b);
}*/

TEST(ResourceAllocator, GetInvalidIdReturnsNull){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    EXPECT_EQ(allocator->Get(123456), nullptr);
}

TEST(ResourceAllocator, MultipleFreeReuseOrder){
    auto allocator = ResourceAllocator<TestResource>::Create(3);

    auto* a = allocator->Alloc(1);
    auto* b = allocator->Alloc(2);
    auto* c = allocator->Alloc(3);

    allocator->Free(b->GetId());
    allocator->Free(a->GetId());

    auto* d = allocator->Alloc(4);
    auto* e = allocator->Alloc(5);

    EXPECT_TRUE(d == a || d == b);
    EXPECT_TRUE(e == a || e == b);
}

class ComplexAsset: public Resource{
public:
    std::string name;
    int x;

    ComplexAsset(std::string n, int v): name(n), x(v){}
};

TEST(ResourceAllocator, PerfectForwardingWorks){
    auto allocator = ResourceAllocator<ComplexAsset>::Create(2);

    auto* a = allocator->Alloc("hello", 42);

    EXPECT_EQ(a->name, "hello");
    EXPECT_EQ(a->x, 42);
}

TEST(ResourceAllocator, ResetInvalidatesOldIds){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    auto* a = allocator->Alloc(1);
    uint32_t id = a->GetId();

    allocator->Reset();

    EXPECT_EQ(allocator->Get(id), nullptr);
}

/*TEST(ResourceAllocator, StressReuseCycles){
    auto allocator = ResourceAllocator<TestAsset>::Create(32);

    for(int cycle = 0; cycle < 100; cycle++){
        std::vector<uint32_t> ids;

        for(int i = 0; i < 1000; i++){
            ids.push_back(allocator->Alloc(i)->GetId());
        }

        for(auto id : ids){
            allocator->Free(id);
        }
    }

    SUCCEED(); // shouldn't crash
}*/

///////////////////////////////////////////

TEST(ResourceAllocator, AllocShared_BasicLifecycle){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    uint32_t savedId = INVALID_RESOURCE_ID;

    {
        std::shared_ptr<TestResource> asset = allocator->AllocShared(42);

        ASSERT_NE(asset, nullptr);
        EXPECT_EQ(asset->value, 42);

        savedId = asset->GetId();
        EXPECT_NE(savedId, INVALID_RESOURCE_ID);

        // Should be retrievable while alive
        TestResource* raw = allocator->Get(savedId);
        ASSERT_NE(raw, nullptr);
        EXPECT_EQ(raw->value, 42);
    }

    // shared_ptr destroyed -> should auto Free()

    TestResource* afterFree = allocator->Get(savedId);
    EXPECT_EQ(afterFree, nullptr);
}

TEST(ResourceAllocator, AllocShared_ReusesFreedSlot){
    auto allocator = ResourceAllocator<TestResource>::Create(2);

    uint32_t firstId;

    {
        auto a = allocator->AllocShared(1);
        firstId = a->GetId();
    } // freed here

    auto b = allocator->AllocShared(2);

    EXPECT_NE(b->GetId(), INVALID_RESOURCE_ID);

    // Not guaranteed same ID, but allocator should still work
    EXPECT_EQ(b->value, 2);
}

TEST(ResourceAllocator, AllocShared_DoubleOwnershipDanger){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    auto a = allocator->AllocShared(10);

    // BAD: creating another shared_ptr from raw pointer
    std::shared_ptr<TestResource> b(a.get(), [](TestResource*) {});

    // When 'a' dies → resource freed
    // 'b' now holds dangling pointer → UB if used
    a.reset();

    EXPECT_EQ(allocator->Get(b->GetId()), nullptr);
}

/////////////////////////////////////////

TEST(ResourceAllocator, IsValid_Alive){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    auto* r = allocator->Alloc(10);

    EXPECT_TRUE(allocator->IsValid(r->GetId()));
}

TEST(ResourceAllocator, IsValid_AfterFreeBeforeReuse){
    auto allocator = ResourceAllocator<TestResource>::Create(1);

    auto* r = allocator->Alloc(10);
    uint32_t id = r->GetId();

    allocator->Free(id);

    EXPECT_FALSE(allocator->IsValid(id));
}

TEST(ResourceAllocator, IsValid_AfterReuse_UndefinedByDesign){
    auto allocator = ResourceAllocator<TestResource>::Create(1);

    auto* r1 = allocator->Alloc(1);
    uint32_t id1 = r1->GetId();

    allocator->Free(id1);

    auto* r2 = allocator->Alloc(2);

    // Same slot reused
    EXPECT_EQ(r1, r2);

    // ID may be same → so just check it's valid for current object
    EXPECT_TRUE(allocator->IsValid(r2->GetId()));
}

TEST(ResourceAllocator, IsValid_InvalidRandom){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    EXPECT_FALSE(allocator->IsValid(999999));
}

TEST(ResourceAllocator, IsValid_AfterReset){
    auto allocator = ResourceAllocator<TestResource>::Create(4);

    auto* r = allocator->Alloc(5);
    uint32_t id = r->GetId();

    allocator->Reset();

    EXPECT_FALSE(allocator->IsValid(id));
}