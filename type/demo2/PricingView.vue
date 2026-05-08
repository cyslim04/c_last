<script setup>
import { computed, ref } from "vue";

const billingCycle = ref("annual");
const expandedFaq = ref(null);

function toggleFaq(index) {
  expandedFaq.value = expandedFaq.value === index ? null : index;
}

const plans = computed(() => [
  {
    id: "free",
    name: "免费版",
    subtitle: "适合个人开发者与小型项目体验",
    monthlyPrice: 0,
    annualPrice: 0,
    cta: "免费开始",
    ctaVariant: "secondary",
    highlight: false,
    features: [
      { text: "最多 3 个项目", included: true },
      { text: "2 名团队成员", included: true },
      { text: "每日 50 条工时记录", included: true },
      { text: "基础交付物管理", included: true },
      { text: "7 天链上存证保留", included: true },
      { text: "社区支持", included: true },
      { text: "自定义品牌", included: false },
      { text: "API 接入", included: false },
      { text: "高级审计报告", included: false },
      { text: "SSO 单点登录", included: false },
    ],
  },
  {
    id: "starter",
    name: "入门版",
    subtitle: "适合初创团队与小型外包项目",
    monthlyPrice: 99,
    annualPrice: 79,
    cta: "开始试用",
    ctaVariant: "secondary",
    highlight: false,
    features: [
      { text: "最多 15 个项目", included: true },
      { text: "10 名团队成员", included: true },
      { text: "每日 500 条工时记录", included: true },
      { text: "完整交付物管理", included: true },
      { text: "30 天链上存证保留", included: true },
      { text: "邮件支持", included: true },
      { text: "自定义品牌", included: false },
      { text: "API 接入", included: false },
      { text: "高级审计报告", included: false },
      { text: "SSO 单点登录", included: false },
    ],
  },
  {
    id: "pro",
    name: "专业版",
    subtitle: "适合成长型公司与多项目并行团队",
    monthlyPrice: 299,
    annualPrice: 239,
    cta: "开始试用",
    ctaVariant: "primary",
    highlight: true,
    badge: "最受欢迎",
    features: [
      { text: "不限项目数量", included: true },
      { text: "50 名团队成员", included: true },
      { text: "不限工时记录", included: true },
      { text: "完整交付物管理 + 自动化", included: true },
      { text: "90 天链上存证保留", included: true },
      { text: "优先邮件 + 工单支持", included: true },
      { text: "自定义品牌", included: true },
      { text: "API 接入", included: true },
      { text: "高级审计报告", included: false },
      { text: "SSO 单点登录", included: false },
    ],
  },
  {
    id: "enterprise",
    name: "企业版",
    subtitle: "适合大型组织与高合规要求场景",
    monthlyPrice: null,
    annualPrice: null,
    cta: "联系销售",
    ctaVariant: "secondary",
    highlight: false,
    features: [
      { text: "不限项目数量", included: true },
      { text: "不限团队成员", included: true },
      { text: "不限工时记录", included: true },
      { text: "完整交付物管理 + 自动化", included: true },
      { text: "永久链上存证保留", included: true },
      { text: "专属客户经理 + 7x24 支持", included: true },
      { text: "自定义品牌", included: true },
      { text: "API 接入 + Webhook", included: true },
      { text: "高级审计报告 + 合规包", included: true },
      { text: "SSO / SAML / OIDC", included: true },
    ],
  },
]);

const featureRows = computed(() => [
  { category: "项目与团队", items: ["项目数量", "团队成员数", "角色权限管理"] },
  { category: "工时与交付", items: ["工时记录上限", "交付物管理", "阶段流程自动化"] },
  { category: "链上存证", items: ["存证保留期限", "多链支持", "存证报告导出"] },
  { category: "审计与合规", items: ["审计报告", "合规认证包", "自定义审计规则"] },
  { category: "集成与支持", items: ["API 接入", "Webhook", "SSO 单点登录", "技术支持等级"] },
]);

const featureValues = computed(() => ({
  "项目数量": ["3 个", "15 个", "不限", "不限"],
  "团队成员数": ["2 名", "10 名", "50 名", "不限"],
  "角色权限管理": ["基础", "基础", "高级", "完全自定义"],
  "工时记录上限": ["50 条/日", "500 条/日", "不限", "不限"],
  "交付物管理": ["基础", "完整", "完整 + 自动化", "完整 + 自动化"],
  "阶段流程自动化": ["—", "—", "支持", "支持"],
  "存证保留期限": ["7 天", "30 天", "90 天", "永久"],
  "多链支持": ["—", "—", "支持", "支持"],
  "存证报告导出": ["—", "基础", "高级", "完整"],
  "审计报告": ["—", "—", "基础", "高级 + 合规包"],
  "合规认证包": ["—", "—", "—", "包含"],
  "自定义审计规则": ["—", "—", "—", "支持"],
  "API 接入": ["—", "—", "REST", "REST + Webhook"],
  "Webhook": ["—", "—", "—", "支持"],
  "SSO 单点登录": ["—", "—", "—", "SAML / OIDC"],
  "技术支持等级": ["社区", "邮件", "优先邮件 + 工单", "7x24 专属经理"],
}));

const faqs = [
  {
    q: "链上存证是什么？我需要了解区块链吗？",
    a: "链上存证是将关键协作数据（如交付确认、审计结论）的哈希值记录到区块链上，形成不可篡改的时间戳证明。平台已封装好底层技术，您无需任何区块链知识即可使用。",
  },
  {
    q: "如何从免费版升级到付费版？",
    a: "在「个人中心 → 订阅管理」中即可自助升级，所有数据无缝迁移，升级立即生效。按年付费可享 8 折优惠。",
  },
  {
    q: "企业版的价格如何确定？",
    a: "企业版按团队规模、存证频率与合规深度定制报价。请联系销售团队获取专属方案与演示。",
  },
  {
    q: "数据安全如何保障？",
    a: "所有传输数据使用 TLS 1.3 加密，静态数据使用 AES-256 加密。链上存证数据分布在多条公链上，确保数据可用性与不可篡改性。",
  },
  {
    q: "支持哪些支付方式？",
    a: "支持支付宝、微信支付、银行转账。企业版支持对公转账与合同签约。",
  },
  {
    q: "可以随时取消订阅吗？",
    a: "可以。按月付费在当周期结束前可随时取消；按年付费支持按剩余月份比例退款。取消后数据可导出并在保留期内下载。",
  },
];
</script>

<template>
  <div class="pricing-view">
    <!-- Hero -->
    <section class="pricing-hero">
      <div class="pricing-hero-copy">
        <div class="home-showcase-kicker home-showcase-kicker-dark">定价方案</div>
        <h2>选择适合团队的协作方案</h2>
        <p class="pricing-hero-lead">
          从个人项目到企业级部署，灵活方案覆盖全场景需求。所有方案均含链上存证核心能力。
        </p>
      </div>

      <!-- Billing toggle -->
      <div class="pricing-billing-toggle">
        <button
          class="pricing-toggle-option"
          :class="{ active: billingCycle === 'monthly' }"
          @click="billingCycle = 'monthly'"
        >
          按月付费
        </button>
        <button
          class="pricing-toggle-option"
          :class="{ active: billingCycle === 'annual' }"
          @click="billingCycle = 'annual'"
        >
          按年付费
          <span class="pricing-save-badge">省 20%</span>
        </button>
      </div>
    </section>

    <!-- Plan cards -->
    <section class="pricing-grid">
      <article
        v-for="plan in plans"
        :key="plan.id"
        class="pricing-card"
        :class="{ highlight: plan.highlight }"
      >
        <div v-if="plan.badge" class="pricing-card-badge">{{ plan.badge }}</div>

        <div class="pricing-card-head">
          <h3>{{ plan.name }}</h3>
          <p>{{ plan.subtitle }}</p>
        </div>

        <div class="pricing-card-price">
          <template v-if="plan.monthlyPrice === null">
            <span class="pricing-currency">¥</span>
            <span class="pricing-amount pricing-amount-custom">定制</span>
          </template>
          <template v-else-if="plan.monthlyPrice === 0">
            <span class="pricing-amount pricing-amount-free">免费</span>
          </template>
          <template v-else>
            <span class="pricing-currency">¥</span>
            <span class="pricing-amount">
              {{ billingCycle === "annual" ? plan.annualPrice : plan.monthlyPrice }}
            </span>
            <span class="pricing-period">/月</span>
          </template>
        </div>

        <button
          class="pricing-cta"
          :class="plan.ctaVariant === 'primary' ? 'pricing-cta-primary' : 'pricing-cta-secondary'"
        >
          {{ plan.cta }}
        </button>

        <ul class="pricing-features">
          <li v-for="(feat, fi) in plan.features" :key="fi" :class="{ excluded: !feat.included }">
            <span class="pricing-feature-icon" :class="{ on: feat.included }">
              {{ feat.included ? "✓" : "—" }}
            </span>
            <span>{{ feat.text }}</span>
          </li>
        </ul>
      </article>
    </section>

    <!-- Feature comparison table -->
    <section class="pricing-compare-section">
      <div class="pricing-compare-head">
        <div class="home-showcase-kicker">功能对比</div>
        <h3>详细功能对比</h3>
      </div>

      <div class="pricing-compare-table-wrap">
        <table class="pricing-compare-table">
          <thead>
            <tr>
              <th class="pricing-compare-feature-col"></th>
              <th v-for="plan in plans" :key="plan.id" class="pricing-compare-plan-col" :class="{ highlight: plan.highlight }">
                {{ plan.name }}
              </th>
            </tr>
          </thead>
          <tbody v-for="row in featureRows" :key="row.category">
            <tr class="pricing-compare-category">
              <td colspan="5">{{ row.category }}</td>
            </tr>
            <tr v-for="item in row.items" :key="item">
              <td class="pricing-compare-feature-name">{{ item }}</td>
              <td
                v-for="(plan, pi) in plans"
                :key="plan.id"
                class="pricing-compare-value"
                :class="{ highlight: plan.highlight }"
              >
                {{ featureValues[item]?.[pi] || "—" }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>

    <!-- FAQ -->
    <section class="pricing-faq-section">
      <div class="pricing-faq-head">
        <div class="home-showcase-kicker">常见问题</div>
        <h3>仍有疑问？</h3>
      </div>

      <div class="pricing-faq-list">
        <div
          v-for="(faq, fi) in faqs"
          :key="fi"
          class="pricing-faq-item"
          :class="{ open: expandedFaq === fi }"
        >
          <button class="pricing-faq-trigger" @click="toggleFaq(fi)">
            <span>{{ faq.q }}</span>
            <span class="pricing-faq-chevron">{{ expandedFaq === fi ? "−" : "+" }}</span>
          </button>
          <div class="pricing-faq-body">
            <p>{{ faq.a }}</p>
          </div>
        </div>
      </div>
    </section>

    <!-- CTA footer -->
    <section class="pricing-cta-footer">
      <h3>准备开始可信协作？</h3>
      <p>免费版无需信用卡，即刻体验链上存证与团队协作。</p>
      <div class="pricing-cta-footer-actions">
        <button class="pricing-cta-primary">免费注册</button>
        <button class="pricing-cta-secondary">预约演示</button>
      </div>
    </section>
  </div>
</template>

<style scoped>
/* ===== Hero ===== */
.pricing-view {
  display: grid;
  gap: 0;
  max-width: 1220px;
  margin: 0 auto;
  padding: 48px 28px 64px;
}

.pricing-hero {
  display: grid;
  gap: 32px;
  padding-bottom: 52px;
  border-bottom: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
}

.pricing-hero-copy {
  display: grid;
  gap: 16px;
  max-width: 640px;
}

.pricing-hero-copy h2 {
  margin: 0;
  color: var(--text, #f3f4f6);
  font-size: clamp(2rem, 4vw, 3.2rem);
  line-height: 1.08;
  letter-spacing: -0.05em;
}

.pricing-hero-lead {
  margin: 0;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  line-height: 1.7;
  font-size: 1.02rem;
  max-width: 520px;
}

/* Billing toggle */
.pricing-billing-toggle {
  display: inline-flex;
  gap: 0;
  border-radius: 10px;
  border: 1px solid var(--soft-border-2, rgba(255, 255, 255, 0.08));
  background: var(--soft-surface-2, rgba(255, 255, 255, 0.035));
  overflow: hidden;
  width: fit-content;
}

.pricing-toggle-option {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  min-height: 44px;
  padding: 0 22px;
  border: 0;
  background: transparent;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.2s ease, color 0.2s ease;
}

.pricing-toggle-option.active {
  background: var(--surface-background, rgba(31, 34, 39, 0.96));
  color: var(--text, #f3f4f6);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.18);
  border-radius: 9px;
}

.pricing-save-badge {
  display: inline-flex;
  align-items: center;
  min-height: 24px;
  padding: 0 8px;
  border-radius: 6px;
  background: rgba(135, 161, 142, 0.18);
  color: var(--success, #87a18e);
  font-size: 11px;
  font-weight: 700;
}

/* ===== Plan cards ===== */
.pricing-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 0;
  border-bottom: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
}

.pricing-card {
  display: grid;
  gap: 20px;
  padding: 36px 28px 32px;
  border-right: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
  position: relative;
  background: transparent;
}

.pricing-card:last-child {
  border-right: 0;
}

.pricing-card.highlight {
  background: linear-gradient(
    180deg,
    rgba(212, 216, 222, 0.05),
    rgba(212, 216, 222, 0.01)
  );
}

.pricing-card.highlight::before {
  content: "";
  position: absolute;
  inset: 0 0 auto 0;
  height: 3px;
  background: linear-gradient(90deg, #d4411e, #c97d5b);
}

.pricing-card-badge {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 12px;
  border-radius: 6px;
  background: rgba(212, 65, 30, 0.14);
  color: #d4411e;
  font-size: 12px;
  font-weight: 700;
  width: fit-content;
}

.pricing-card-head {
  display: grid;
  gap: 6px;
}

.pricing-card-head h3 {
  margin: 0;
  font-size: 1.18rem;
  line-height: 1.2;
  color: var(--text, #f3f4f6);
}

.pricing-card-head p {
  margin: 0;
  font-size: 13px;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  line-height: 1.5;
}

.pricing-card-price {
  display: flex;
  align-items: baseline;
  gap: 2px;
  min-height: 56px;
}

.pricing-currency {
  font-size: 1.2rem;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  align-self: flex-start;
  margin-top: 6px;
}

.pricing-amount {
  font-size: 2.8rem;
  font-weight: 800;
  line-height: 1;
  letter-spacing: -0.04em;
  color: var(--text, #f3f4f6);
}

.pricing-amount-free {
  font-size: 2.2rem;
}

.pricing-amount-custom {
  font-size: 2rem;
}

.pricing-period {
  font-size: 0.95rem;
  color: var(--muted, rgba(169, 175, 184, 0.8));
}

.pricing-cta {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 46px;
  padding: 0 20px;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
  transition: background 0.2s ease, border-color 0.2s ease, opacity 0.2s ease;
  border: 1px solid transparent;
  width: 100%;
}

.pricing-cta-primary {
  background: #d4411e;
  border-color: #d4411e;
  color: #f5f3ef;
}

.pricing-cta-primary:hover {
  opacity: 0.9;
}

.pricing-cta-secondary {
  background: transparent;
  border-color: var(--soft-border-2, rgba(255, 255, 255, 0.08));
  color: var(--text, #f3f4f6);
}

.pricing-cta-secondary:hover {
  border-color: var(--border-strong, rgba(255, 255, 255, 0.14));
  background: rgba(255, 255, 255, 0.03);
}

.pricing-features {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  gap: 12px;
}

.pricing-features li {
  display: flex;
  align-items: flex-start;
  gap: 10px;
  font-size: 13px;
  line-height: 1.5;
  color: var(--text, #f3f4f6);
}

.pricing-features li.excluded {
  color: var(--muted, rgba(169, 175, 184, 0.55));
}

.pricing-feature-icon {
  flex: 0 0 auto;
  width: 18px;
  text-align: center;
  font-weight: 700;
  font-size: 13px;
  margin-top: 1px;
}

.pricing-feature-icon.on {
  color: var(--success, #87a18e);
}

/* ===== Feature comparison ===== */
.pricing-compare-section {
  display: grid;
  gap: 32px;
  padding: 64px 0;
  border-bottom: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
}

.pricing-compare-head {
  display: grid;
  gap: 10px;
}

.pricing-compare-head h3 {
  margin: 0;
  font-size: clamp(1.5rem, 2.5vw, 2.2rem);
  line-height: 1.1;
  letter-spacing: -0.04em;
  color: var(--text, #f3f4f6);
}

.pricing-compare-table-wrap {
  overflow-x: auto;
  border: 1px solid var(--soft-border-2, rgba(255, 255, 255, 0.08));
  border-radius: 12px;
}

.pricing-compare-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.pricing-compare-table thead th {
  padding: 16px 20px;
  text-align: left;
  font-weight: 700;
  color: var(--text, #f3f4f6);
  background: var(--soft-surface-2, rgba(255, 255, 255, 0.035));
  border-bottom: 1px solid var(--soft-border-2, rgba(255, 255, 255, 0.08));
}

.pricing-compare-table thead th.highlight {
  background: rgba(212, 65, 30, 0.06);
}

.pricing-compare-feature-col {
  width: 28%;
  min-width: 180px;
}

.pricing-compare-plan-col {
  width: 18%;
  min-width: 120px;
}

.pricing-compare-category td {
  padding: 14px 20px 8px;
  color: var(--subtle, #a9afb8);
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  background: transparent;
}

.pricing-compare-feature-name {
  padding: 10px 20px;
  color: var(--text, #f3f4f6);
  border-top: 1px solid rgba(255, 255, 255, 0.04);
}

.pricing-compare-value {
  padding: 10px 20px;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  border-top: 1px solid rgba(255, 255, 255, 0.04);
  text-align: left;
}

.pricing-compare-value.highlight {
  background: rgba(212, 65, 30, 0.02);
}

/* ===== FAQ ===== */
.pricing-faq-section {
  display: grid;
  gap: 32px;
  padding: 64px 0;
  border-bottom: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
}

.pricing-faq-head {
  display: grid;
  gap: 10px;
}

.pricing-faq-head h3 {
  margin: 0;
  font-size: clamp(1.5rem, 2.5vw, 2.2rem);
  line-height: 1.1;
  letter-spacing: -0.04em;
  color: var(--text, #f3f4f6);
}

.pricing-faq-list {
  display: grid;
  gap: 0;
  border: 1px solid var(--soft-border-2, rgba(255, 255, 255, 0.08));
  border-radius: 12px;
  overflow: hidden;
}

.pricing-faq-item {
  border-bottom: 1px solid var(--soft-border-1, rgba(255, 255, 255, 0.06));
}

.pricing-faq-item:last-child {
  border-bottom: 0;
}

.pricing-faq-item.open .pricing-faq-trigger {
  background: var(--soft-surface-2, rgba(255, 255, 255, 0.035));
}

.pricing-faq-trigger {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 20px 24px;
  border: 0;
  background: transparent;
  color: var(--text, #f3f4f6);
  font-size: 15px;
  font-weight: 600;
  text-align: left;
  cursor: pointer;
  transition: background 0.2s ease;
}

.pricing-faq-trigger:hover {
  background: var(--soft-surface-1, rgba(255, 255, 255, 0.02));
}

.pricing-faq-chevron {
  flex: 0 0 auto;
  width: 28px;
  height: 28px;
  display: grid;
  place-items: center;
  border-radius: 6px;
  background: var(--soft-surface-3, rgba(255, 255, 255, 0.045));
  font-size: 18px;
  color: var(--subtle, #a9afb8);
}

.pricing-faq-body {
  display: grid;
  max-height: 0;
  overflow: hidden;
  transition: max-height 0.32s ease, padding 0.32s ease;
}

.pricing-faq-item.open .pricing-faq-body {
  max-height: 300px;
}

.pricing-faq-body p {
  margin: 0;
  padding: 0 24px 20px;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  line-height: 1.75;
  font-size: 14px;
}

/* ===== CTA Footer ===== */
.pricing-cta-footer {
  display: grid;
  gap: 20px;
  justify-items: center;
  text-align: center;
  padding: 64px 0 32px;
}

.pricing-cta-footer h3 {
  margin: 0;
  font-size: clamp(1.5rem, 2.5vw, 2.2rem);
  line-height: 1.1;
  letter-spacing: -0.04em;
  color: var(--text, #f3f4f6);
}

.pricing-cta-footer > p {
  margin: 0;
  color: var(--muted, rgba(169, 175, 184, 0.8));
  line-height: 1.7;
  max-width: 440px;
}

.pricing-cta-footer-actions {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  justify-content: center;
}

/* ===== Responsive ===== */
@media (max-width: 1100px) {
  .pricing-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .pricing-card:nth-child(2n) {
    border-right: 0;
  }

  .pricing-card:nth-child(-n + 2) {
    border-bottom: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
  }
}

@media (max-width: 720px) {
  .pricing-view {
    padding: 32px 20px 48px;
  }

  .pricing-grid {
    grid-template-columns: 1fr;
  }

  .pricing-card {
    border-right: 0;
    border-bottom: 1px solid var(--home-editorial-line, rgba(255, 255, 255, 0.08));
  }

  .pricing-card:last-child {
    border-bottom: 0;
  }

  .pricing-hero {
    padding-bottom: 36px;
    gap: 24px;
  }

  .pricing-compare-section,
  .pricing-faq-section {
    padding: 44px 0;
  }

  .pricing-cta-footer {
    padding-top: 44px;
  }

  .pricing-billing-toggle {
    width: 100%;
  }

  .pricing-toggle-option {
    flex: 1;
    justify-content: center;
    padding: 0 14px;
  }
}
</style>
